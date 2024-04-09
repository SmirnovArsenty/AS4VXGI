#include "voxel.fx"

uint get_heap_parent_index(uint index)
{
    return (max(index, 1u) - 1u) / 2u;
}

bool inBoxBounds(MeshTreeNode mesh_node, float3 position)
{
    float3 diag = mesh_node.max - mesh_node.min;
    float length = sqrt(dot(diag, diag));
    bool inside = true;
    [unroll]
    for(int i = 0; i < 3; i++) {
        inside = inside && (position[i] > mesh_node.min[i] - length * 2);
        inside = inside && (position[i] < mesh_node.max[i] + length * 2);
    }

    return inside;
}

float boxIntersection(Ray ray, MeshTreeNode mesh_node)
{
    float3 _min = mul(MODEL_MATRICES[0], float4(mesh_node.min, 1.f)).xyz;
    float3 _max = mul(MODEL_MATRICES[0], float4(mesh_node.max, 1.f)).xyz;

    float epsilon = 0.00001f;
    if (ray.direction.x == 0) {
        ray.direction.x = epsilon;
    }
    if (ray.direction.y == 0) {
        ray.direction.y = epsilon;
    }
    if (ray.direction.z == 0) {
        ray.direction.z = epsilon;
    }

    float coeffs[6];
    coeffs[0] = (_min.x - ray.origin.x) / (ray.direction.x);
    coeffs[1] = (_min.y - ray.origin.y) / (ray.direction.y);
    coeffs[2] = (_min.z - ray.origin.z) / (ray.direction.z);
    coeffs[3] = (_max.x - ray.origin.x) / (ray.direction.x);
    coeffs[4] = (_max.y - ray.origin.y) / (ray.direction.y);
    coeffs[5] = (_max.z - ray.origin.z) / (ray.direction.z);

    float t = 0;
    [unroll]
    for (int i = 0; i < 6; ++i) {
        if (coeffs[i] > 0 && inBoxBounds(mesh_node, ray.origin + ray.direction * coeffs[i])) {
            if (t == 0 || coeffs[i] < t){
                t = coeffs[i];
            }
        }
    }

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

#define OPTIMIZATION 1

#define BITS_COUNT 32
#define ARRAY_SIZE 32
[numthreads(4, 4, 4)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
#if OPTIMIZATION
    uint hit_buffer[ARRAY_SIZE][3] = (uint[ARRAY_SIZE][3])0; // [optimization] 32-bit buffer to store box hits
#endif
    uint count_parents_not_found = 0;

    uint3 currentIndex = dispatchThreadID;
    float t = 0;
    float3 normal = (0).xxx;
    float box_t = 0;
    Ray rays[3] = { GenerateRayForward(currentIndex), GenerateRayRight(currentIndex), GenerateRayUp(currentIndex) };
    bool found = false;
    for (uint ray_index = 0; ray_index < 3; ++ray_index) {
        for (uint i = 0; i < voxelGrid.mesh_node_count; ++i) {
#if OPTIMIZATION
            uint parent_index = (max(i, 1u) - 1u) / 2u;
            // for (parent_index = (max(i, 1u) - 1u) / 2u; parent_index / BITS_COUNT > ARRAY_SIZE; ++count_parents_not_found) {
            //     parent_index = (max(parent_index, 1u) - 1u) / 2u;
            // }
            bool need_check = true;
            if (parent_index / BITS_COUNT < ARRAY_SIZE) {
                uint parent_hit_bit = (1U << (parent_index - (parent_index / BITS_COUNT) * BITS_COUNT));
                need_check = (i == 0) || (hit_buffer[parent_index / BITS_COUNT][ray_index] & parent_hit_bit);
            }
#else
            bool need_check = true;
#endif

            bool box_intersected = false;
            float box_distance = 0;
            float tri_t = 0;
            float3 tri_normal = (0).xxx;
            { // check box intersection
                box_intersected = false;
                if (!need_check) {
                    continue;
                }
                float box_distance = boxIntersection(rays[ray_index], MESH_TREE[i]);
                if (box_distance <= 0) { // || box_distance > voxelGrid.size / voxelGrid.dimension) {
                    if (i == 0) {
                        break;
                    }
                    continue;
                }
                box_t = box_distance;
            }
            {
                for (int j = MESH_TREE[i].start_index; j < MESH_TREE[i].start_index + MESH_TREE[i].count; j += 3) {
                    float3 _tri_normal = (0).xxx;
                    float _tri_t = triangleIntersection(rays[ray_index], mul(MODEL_MATRICES[0], float4(VERTICES[INDICES[j + 0]].position, 1.f)).xyz,
                                                             mul(MODEL_MATRICES[0], float4(VERTICES[INDICES[j + 1]].position, 1.f)).xyz,
                                                             mul(MODEL_MATRICES[0], float4(VERTICES[INDICES[j + 2]].position, 1.f)).xyz,
                                                             VERTICES[INDICES[j + 0]].normal,
                                                             VERTICES[INDICES[j + 1]].normal,
                                                             VERTICES[INDICES[j + 2]].normal,
                                                             _tri_normal);
                    if (_tri_t > 0 && (_tri_t < t || t == 0) && (_tri_t < voxelGrid.size / voxelGrid.dimension)) {
                        t = _tri_t;
                        normal = _tri_normal;
                    }
                }
            }
#if OPTIMIZATION
            uint hit_index = i / BITS_COUNT;
            if (hit_index < ARRAY_SIZE) {
                hit_buffer[hit_index][ray_index] |= (1U << (i - hit_index * BITS_COUNT));
            }
#endif
            if (tri_t > 0 && (tri_t < t || t == 0)) {
                box_t = box_distance;
                t = tri_t;
                normal = tri_normal;
                found = true;
            }
        }
        if (found) {
            break;
        }
    }

    if (t > 0) {
        Voxel voxel = (Voxel)0;
        voxel.albedo.x = currentIndex.x;
        voxel.albedo.y = currentIndex.y;
        voxel.albedo.z = currentIndex.z;
        // voxel.normal.x = rays[1].direction.x;
        // voxel.normal.y = rays[1].direction.y;
        // voxel.normal.z = rays[1].direction.z;
        voxel.normal = normal;
        voxel.metalness = t;
        voxel.sharpness = asfloat(count_parents_not_found);
        VOXELS[currentIndex.x + currentIndex.y * voxelGrid.dimension + currentIndex.z * voxelGrid.dimension * voxelGrid.dimension] = voxel;
    }
}
