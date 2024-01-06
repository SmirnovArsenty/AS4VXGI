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

#define EPSILON 0.000001f
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
    float3 _min = mul(model_matrices[0], float4(mesh_node.min, 1.f)).xyz;
    float3 _max = mul(model_matrices[0], float4(mesh_node.max, 1.f)).xyz;

    float coeffs[6];
    coeffs[0] = (_min.x - ray.origin.x) / (ray.direction.x);
    coeffs[1] = (_min.y - ray.origin.y) / (ray.direction.y);
    coeffs[2] = (_min.z - ray.origin.z) / (ray.direction.z);
    coeffs[3] = (_max.x - ray.origin.x) / (ray.direction.x);
    coeffs[4] = (_max.y - ray.origin.y) / (ray.direction.y);
    coeffs[5] = (_max.z - ray.origin.z) / (ray.direction.z);

    float t = 0;
    [unroll]
    for (int i = 0; i < 6; ++i)
        t = (coeffs[i] > EPSILON && inBoxBounds(mesh_node, ray.origin + ray.direction * coeffs[i])) ? (t == 0 ? coeffs[i] : min(coeffs[i], t)) : t;

    return t;
}

float triangleIntersection(in Ray ray, float3 v0, float3 v1, float3 v2, float3 normal1, float3 normal2, float3 normal3, inout float3 o_normal)
{
    float3 edge1 = v1 - v0;
    float3 edge2 = v2 - v0;
    float3 _u1 = (edge1 * (dot(edge2, edge2)) - edge2 * (dot(edge1, edge2))) / ((dot(edge1, edge1)) * (dot(edge2, edge2)) - (dot(edge1, edge2)) * (dot(edge1, edge2)));
    float3 _v1 = (edge2 * (dot(edge1, edge1)) - edge1 * (dot(edge1, edge2))) / ((dot(edge1, edge1)) * (dot(edge2, edge2)) - (dot(edge1, edge2)) * (dot(edge1, edge2)));
    float u = dot(v0, _u1);
    float v = dot(v0, _v1);

    float3 normal = normalize(cross(normalize(v1 - v0), normalize(v2 - v0)));

    float dn = dot(ray.direction, normal);

    float t = 0;

    if (dn != 0) {
        float temp = -(dot(ray.origin, normal) - dot(v0, normal)) / (dn); // - EPSILON
        if (temp > 0) {
            float3 p = ray.origin + ray.direction * temp;
            float _u = dot(p, _u1) - u;
            float _v = dot(p, _v1) - v;
            float _w = (1 - _v - _u);

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

void intersectMeshNode(in Ray ray, in uint index, in bool check_needed, inout float box_t, out bool box_intersected, inout float t, inout float3 normal)
{
    if (!check_needed) {
        return;
    }
    MeshTreeNode mesh_node = mesh_tree[index];
    float box_distance = boxIntersection(ray, mesh_node);
    if (box_distance < EPSILON) { // || box_distance > voxel_grid.size / voxel_grid.dimension) {
        return;
    }
    box_t = box_distance;

    // [optimization] mark box hit
    box_intersected = true;

    for (int j = mesh_node.start_index; j < mesh_node.start_index + mesh_node.count; j += 3) {
        float3 tri_normal = (0).xxx;
        float tri_t = triangleIntersection(ray, mul(model_matrices[0], float4(vertices[indices[j + 0]].position, 1.f)).xyz,
                                                mul(model_matrices[0], float4(vertices[indices[j + 1]].position, 1.f)).xyz,
                                                mul(model_matrices[0], float4(vertices[indices[j + 2]].position, 1.f)).xyz,
                                                vertices[indices[j + 0]].normal,
                                                vertices[indices[j + 1]].normal,
                                                vertices[indices[j + 2]].normal,
                                                tri_normal);
        if (tri_t > EPSILON && (tri_t < t || t == 0) && (tri_t < voxel_grid.size / voxel_grid.dimension)) {
            t = tri_t;
            normal = tri_normal;
        }
    }
}

#define BITS_COUNT 32
#define ARRAY_SIZE 1000
[numthreads(1, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint hit_buffer[ARRAY_SIZE][3] = (uint[ARRAY_SIZE][3])0; // [optimization] 32-bit buffer to store box hits

    float t = 0;
    float3 normal = (0).xxx;
    float box_t = 0;
    Ray rays[3] = { GenerateRayForward(dispatchThreadID), GenerateRayRight(dispatchThreadID), GenerateRayUp(dispatchThreadID) };
    for (uint ray_index = 0; ray_index < 3; ++ray_index) {
        for (uint i = 0; i < voxel_grid.mesh_node_count; ++i) {
            uint parent_index = (max(i, 1u) - 1u) / 2u;
            uint parent_hit_index = parent_index / BITS_COUNT;
            uint parent_hit_index_bit_offset = parent_index - parent_hit_index * BITS_COUNT;
            uint parent_hit_bit = (1 << parent_hit_index_bit_offset);
            bool need_check = (i == 0) || (hit_buffer[parent_hit_index][ray_index] & parent_hit_bit) || true;

            bool box_intersected = false;
            float box_distance = 0;
            float tri_t = 0;
            float3 tri_normal = (0).xxx;
            intersectMeshNode(rays[ray_index], i, need_check, box_distance, box_intersected, tri_t, tri_normal);
            if (box_intersected) {
                uint hit_index = i / BITS_COUNT;
                if (hit_index < ARRAY_SIZE) {
                    hit_buffer[hit_index][ray_index] |= (1 << (i - hit_index * BITS_COUNT));
                }
                if (tri_t > EPSILON && (tri_t < t || t == 0)) {
                    box_t = box_distance;
                    t = tri_t;
                    normal = tri_normal;
                }
            }
        }
    }
    Voxel voxel = (Voxel)0;
    voxel.albedo.x = dispatchThreadID.x;
    voxel.albedo.y = dispatchThreadID.y;
    voxel.albedo.z = dispatchThreadID.z;
    voxel.normal = normal;
    voxel.metalness = t;
    voxel.sharpness = box_t;
    voxels[dispatchThreadID.x + dispatchThreadID.y * voxel_grid.dimension + dispatchThreadID.z * voxel_grid.dimension * voxel_grid.dimension] = voxel;
}
