#include "voxel.fx"

float4 pack_voxel(Voxel voxel)
{
    float4 result;
    result.x = asfloat(f32tof16(voxel.normal.x) | (f32tof16(voxel.normal.y) << 16));
    result.y = asfloat(f32tof16(voxel.normal.z) | (f32tof16(voxel.sharpness) << 16));
    result.z = asfloat(f32tof16(voxel.albedo.x) | (f32tof16(voxel.albedo.y) << 16));
    result.w = asfloat(f32tof16(voxel.albedo.z) | (f32tof16(voxel.metalness) << 16));
    return result;
}

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

float boxIntersection(in Ray rays[3], MeshTreeNode mesh_node)
{
    float3 _min = mul(MODEL_MATRICES[0], float4(mesh_node.min, 1.f)).xyz;
    float3 _max = mul(MODEL_MATRICES[0], float4(mesh_node.max, 1.f)).xyz;

    float epsilon = 0.00001f;

    [unroll]
    for (int i = 0; i < 3; ++i) {
        if (rays[i].direction.x == 0) {
            rays[i].direction.x = epsilon;
        }
        if (rays[i].direction.y == 0) {
            rays[i].direction.y = epsilon;
        }
        if (rays[i].direction.z == 0) {
            rays[i].direction.z = epsilon;
        }

    
        float coeffs[6];
        coeffs[0] = (_min.x - rays[i].origin.x) / (rays[i].direction.x);
        coeffs[1] = (_min.y - rays[i].origin.y) / (rays[i].direction.y);
        coeffs[2] = (_min.z - rays[i].origin.z) / (rays[i].direction.z);
        coeffs[3] = (_max.x - rays[i].origin.x) / (rays[i].direction.x);
        coeffs[4] = (_max.y - rays[i].origin.y) / (rays[i].direction.y);
        coeffs[5] = (_max.z - rays[i].origin.z) / (rays[i].direction.z);

        float t = 0;
        [unroll]
        for (int j = 0; j < 6; ++j) {
            if (coeffs[j] > 0 && inBoxBounds(mesh_node, rays[i].origin + rays[i].direction * coeffs[j])) {
                if (t == 0 || coeffs[j] < t) {
                    t = coeffs[j];
                }
            }
        }

        if (t > 0) {
            return t;
        }
    }
    return 0;
}

float triangleIntersection(in Ray rays[3], in float3 v0, in float3 v1, in float3 v2, in float3 normal1, in float3 normal2, in float3 normal3, out float3 o_normal)
{
    float3 edge1 = v1 - v0;
    float3 edge2 = v2 - v0;
    float3 _u1 = (edge1 * (dot(edge2, edge2)) - edge2 * (dot(edge1, edge2))) / ((dot(edge1, edge1)) * (dot(edge2, edge2)) - (dot(edge1, edge2)) * (dot(edge1, edge2)));
    float3 _v1 = (edge2 * (dot(edge1, edge1)) - edge1 * (dot(edge1, edge2))) / ((dot(edge1, edge1)) * (dot(edge2, edge2)) - (dot(edge1, edge2)) * (dot(edge1, edge2)));
    float u = dot(v0, _u1);
    float v = dot(v0, _v1);

    float3 normal = normalize(cross(normalize(v1 - v0), normalize(v2 - v0)));

    for (int i = 0; i < 3; ++i) {
        float dn = dot(rays[i].direction, normal);
        if (dn == 0) {
            continue;
        }

        float t = -(dot(rays[i].origin, normal) - dot(v0, normal)) / (dn); // - EPSILON
        if (t <= 0) {
            continue;
        }

        float3 p = rays[i].origin + rays[i].direction * t;
        
        float _u = dot(p, _u1) - u;
        if (_u <= 0 || _u >= 1) {
            continue;
        }

        float _v = dot(p, _v1) - v;
        if (_v <= 0 || _v >= 1) {
            continue;
        }
        if (_u + _v >= 1) {
            continue;
        }

        float _w = (1 - _v - _u);
        o_normal = normal1 * _w + normal2 * _u + normal3 * _v;

        return t;
    }
    return 0;
}

#define OPTIMIZATION 1


#define BITS_COUNT 32
#define ARRAY_SIZE 32
[numthreads(4, 4, 4)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint hit_buffer[ARRAY_SIZE] = (uint[ARRAY_SIZE])0; // [optimization] 32-bit buffer to store box hits

    uint3 currentIndex = dispatchThreadID;
    float t = 0;
    float3 normal = (0).xxx;
    float box_t = 0;
    Ray rays[3] = { GenerateRayForward(currentIndex), GenerateRayRight(currentIndex), GenerateRayUp(currentIndex) };
    bool found = false;

    for (uint i = 0; i < voxelGrid.mesh_node_count; ++i) {
        uint parent_index = (max(i, 1u) - 1u) / 2u;
        while (parent_index / BITS_COUNT >= ARRAY_SIZE) {
            parent_index = (max(parent_index, 1u) - 1u) / 2u;
        }
        uint parent_hit_bit = (1U << (parent_index - (parent_index / BITS_COUNT) * BITS_COUNT));
        bool need_check = (i == 0) || (hit_buffer[parent_index / BITS_COUNT] & parent_hit_bit);
        if (!need_check) {
            continue;
        }

        {
            bool box_intersected = false;
            float box_distance = 0;
            float tri_t = 0;
            float3 tri_normal = (0).xxx;
            { // check box intersection
                box_intersected = false;
                float box_distance = boxIntersection(rays, MESH_TREE[i]);
                if (box_distance <= 0) { // || box_distance > voxelGrid.size / voxelGrid.dimension) {
                    if (i == 0)
                    {
                        break;
                    }
                    continue;
                }
                box_t = box_distance;
            }

            uint hit_index = i / BITS_COUNT;
            if (hit_index < ARRAY_SIZE) {
                hit_buffer[hit_index] |= (1U << (i - hit_index * BITS_COUNT));
            }

            for (int j = MESH_TREE[i].start_index; j < MESH_TREE[i].start_index + MESH_TREE[i].count; j += 3) {
                float3 _tri_normal = (0).xxx;
                Vertex vertices[3];
                vertices[0] = VERTICES[INDICES[j + 0]];
                vertices[1] = VERTICES[INDICES[j + 1]];
                vertices[2] = VERTICES[INDICES[j + 2]];
                
                float _tri_t = triangleIntersection(rays,
                                                    mul(MODEL_MATRICES[0], float4(vertices[0].position, 1.f)).xyz,
                                                    mul(MODEL_MATRICES[0], float4(vertices[1].position, 1.f)).xyz,
                                                    mul(MODEL_MATRICES[0], float4(vertices[1].position, 1.f)).xyz,
                                                    vertices[0].normal,
                                                    vertices[1].normal,
                                                    vertices[2].normal,
                                                    _tri_normal);
                if (_tri_t > 0 && (_tri_t < t || t == 0) && (_tri_t < voxelGrid.size / voxelGrid.dimension)) {
                    t = _tri_t;
                    normal = _tri_normal;
                }
            }

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
        voxel.sharpness = 0;

        VOXELS[uint3(currentIndex.x, currentIndex.y, currentIndex.z)] = pack_voxel(voxel);
    }
}
