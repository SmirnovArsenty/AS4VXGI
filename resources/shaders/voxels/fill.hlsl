#define CAMERA_REGISTER b0
#define VOXEL_GRID_REGISTER b1
#include "resources/shaders/voxels/voxel.fx"

// u- slots
RWStructuredBuffer<Voxel> voxels : register(u0);

// t- slots
StructuredBuffer<MeshTreeNode> mesh_tree : register(t0);
StructuredBuffer<int> indices : register(t1);
StructuredBuffer<Vertex> vertices : register(t2);
StructuredBuffer<float4x4> model_matrices : register(t3);

#define EPSILON 0.001f
bool inBoxBounds(MeshTreeNode mesh_node, float3 position)
{
    bool inside = true;
    [unroll]
    for(int i = 0; i < 3; i++) {
        inside = inside && (position[i] > mesh_node.min[i] - EPSILON);
        inside = inside && (position[i] < mesh_node.max[i] + EPSILON);
    }

    return inside;
}

float boxIntersection(Ray ray, MeshTreeNode mesh_node)
{
    float3 corner0 = mul(model_matrices[0], float4(mesh_node.min, 1.f)).xyz;
    float3 corner1 = mul(model_matrices[0], float4(mesh_node.max, 1.f)).xyz;

    float coeffs[6];
    coeffs[0] = (corner0.x - ray.origin.x) / (ray.direction.x);
    coeffs[1] = (corner0.y - ray.origin.y) / (ray.direction.y);
    coeffs[2] = (corner0.z - ray.origin.z) / (ray.direction.z);
    coeffs[3] = (corner1.x - ray.origin.x) / (ray.direction.x);
    coeffs[4] = (corner1.y - ray.origin.y) / (ray.direction.y);
    coeffs[5] = (corner1.z - ray.origin.z) / (ray.direction.z);

    float t = 0;
    [unroll]
    for (int i = 0; i < 6; ++i)
        t = (coeffs[i] > EPSILON && inBoxBounds(mesh_node, ray.origin + ray.direction * coeffs[i])) ? (t == 0 ? coeffs[i] : min(coeffs[i], t)) : t;

    return t;
}

float triangleIntersection(Ray ray, float3 v0, float3 v1, float3 v2, float3 normal1, float3 normal2, float3 normal3, out float3 o_normal)
{
    float3 edge1 = v1 - v0;
    float3 edge2 = v2 - v0;
    float3 _u1 = (edge1 * (dot(edge2, edge2)) - edge2 * (dot(edge1, edge2))) / ((dot(edge1, edge1)) * (dot(edge2, edge2)) - (dot(edge1, edge2)) * (dot(edge1, edge2)));
    float3 _v1 = (edge2 * (dot(edge1, edge1)) - edge1 * (dot(edge1, edge2))) / ((dot(edge1, edge1)) * (dot(edge2, edge2)) - (dot(edge1, edge2)) * (dot(edge1, edge2)));
    float u = dot(v0, _u1);
    float v = dot(v0, _v1);
    float3 normal = normalize(cross(v1 - v0, v2 - v0));

    float t = -1;
    o_normal = (float3)0;

    float dn = dot(ray.direction, normal);
    if (dn != 0) {
        float temp = -(dot(ray.origin, normal) - dot(v0, normal)) / (dn) - 1e-5;
        if (temp > 0) {
            float3 res = ray.origin + ray.direction * t;
            float _u = dot(res, _u1) - u;
            float _v = dot(res, _v1) - v;
            float _w = (1 - v - u);

            if (_u > 0 && _u < 1) {
                if (_v > 0 && _v < 1) {
                    if (_u + _v < 1) {
                        t = temp;

                        o_normal = normal1 * _w + normal2 * _u + normal3 * _v;
                    }
                }
            }
        }
    }
    return t;
}

[numthreads(1, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const uint bits_count = 32;
    dword hit_buffer[4000]; // [optimization] 32-bit buffer to store box hits
    for (uint k = 0; k < 4000; ++k) {
        hit_buffer[k] = (dword)0;
    }
    uint count_skiped = 0; // [optimization] debug counter

    Ray ray = GenerateRay(dispatchThreadID);
    float t = -1;
    bool found = false;
    Vertex tri_intersected[3];
    float3 tri_normal;
    float box_t = -1;
    for (uint i = 0; i < (uint)voxel_grid.mesh_node_count; ++i) {
        bool need_check = true;
        // if (i != 0)
        // { // [optimization]
        //     uint parent_index = (i - 1u) / 2u;
        //     uint parent_hit_index = parent_index / bits_count;
        //     while (parent_hit_index >= 4000) {
        //         parent_index = (parent_index - 1u) / 2u;
        //         parent_hit_index = parent_index / bits_count;
        //     }
        //     if (hit_buffer[parent_hit_index] & (1 << (parent_index - parent_hit_index * bits_count)) == 0) {
        //         // skip current box, because parent was not intersected
        //         ++count_skiped;
        //         need_check = false;
        //     }
        // }

        if (need_check) {
            MeshTreeNode mesh_node = mesh_tree[i];
            float box_distance = boxIntersection(ray, mesh_node);
            if (box_distance > EPSILON) { // && box_t < voxel_grid.size * 2) {
                if (box_t == -1 || box_distance < box_t) {
                    box_t = box_distance;
                }
                { // [optimization]
                    // mark box hit
                    uint hit_index = i / bits_count;
                    if (hit_index < 4000) {
                        hit_buffer[hit_index] |= (1 << (i - hit_index * bits_count));
                    }
                }
                for (int j = mesh_node.start_index; j < mesh_node.start_index + mesh_node.count; ++j) {
                    Vertex tri[3] = { vertices[indices[j + 0]], vertices[indices[j + 1]], vertices[indices[j + 2]] };
                    float3 normal;
                    float tri_t = triangleIntersection(ray, mul(model_matrices[0], float4(vertices[indices[j + 0]].position, 1.f)).xyz,
                                                            mul(model_matrices[0], float4(vertices[indices[j + 1]].position, 1.f)).xyz,
                                                            mul(model_matrices[0], float4(vertices[indices[j + 2]].position, 1.f)).xyz,
                                                            vertices[indices[j + 0]].normal,
                                                            vertices[indices[j + 1]].normal,
                                                            vertices[indices[j + 2]].normal,
                                                            normal);
                    if (tri_t > EPSILON && (tri_t < t || t == -1) && (tri_t < voxel_grid.size / voxel_grid.dimension)) {
                        t = tri_t;
                        tri_intersected[0] = tri[0];
                        tri_intersected[1] = tri[1];
                        tri_intersected[2] = tri[2];
                        tri_normal = normal;
                        found = true;
                    }
                }
            }
        }
    }
    Voxel voxel = (Voxel)0;
    if (found) {
        float3 edge1 = tri_intersected[1].position - tri_intersected[0].position;
        float3 edge2 = tri_intersected[2].position - tri_intersected[0].position;
        voxel.normal = normalize(cross(edge1, edge2));
    }
    voxel.albedo.x = dispatchThreadID.x;
    voxel.albedo.y = dispatchThreadID.y;
    voxel.albedo.z = dispatchThreadID.z;
    voxel.metalness = t;
    voxel.sharpness = box_t;
    voxels[dispatchThreadID.x + dispatchThreadID.y * voxel_grid.dimension + dispatchThreadID.z * voxel_grid.dimension * voxel_grid.dimension] = voxel;
}
