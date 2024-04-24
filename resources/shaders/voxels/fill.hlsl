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

Voxel unpack_voxel(float4 data)
{
    Voxel result;
    result.normal.x = f16tof32(asuint(data.x) & 0xFFFF);
    result.normal.y = f16tof32((asuint(data.x) >> 16) & 0xFFFF);
    result.normal.z = f16tof32(asuint(data.y) & 0xFFFF);
    result.sharpness = f16tof32((asuint(data.y) >> 16) & 0xFFFF);

    result.albedo.x = f16tof32(asuint(data.z) & 0xFFFF);
    result.albedo.y = f16tof32((asuint(data.z) >> 16) & 0xFFFF);
    result.albedo.z = f16tof32(asuint(data.w) & 0xFFFF);
    result.metalness = f16tof32((asuint(data.w) >> 16) & 0xFFFF);
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

float boxIntersection(in Ray ray[3], MeshTreeNode mesh_node)
{
    float3 _min = mul(MODEL_MATRICES[0], float4(mesh_node.min, 1.f)).xyz;
    float3 _max = mul(MODEL_MATRICES[0], float4(mesh_node.max, 1.f)).xyz;

    float epsilon = 0.000001f;
    [unroll]
    for (int ray_index = 0; ray_index < 3; ++ray_index) {
        if (ray[ray_index].direction.x == 0) {
            ray[ray_index].direction.x = epsilon;
        }
        if (ray[ray_index].direction.y == 0) {
            ray[ray_index].direction.y = epsilon;
        }
        if (ray[ray_index].direction.z == 0) {
            ray[ray_index].direction.z = epsilon;
        }

        float coeffs[6];
        coeffs[0] = (_min.x - ray[ray_index].origin.x) / (ray[ray_index].direction.x);
        coeffs[1] = (_min.y - ray[ray_index].origin.y) / (ray[ray_index].direction.y);
        coeffs[2] = (_min.z - ray[ray_index].origin.z) / (ray[ray_index].direction.z);
        coeffs[3] = (_max.x - ray[ray_index].origin.x) / (ray[ray_index].direction.x);
        coeffs[4] = (_max.y - ray[ray_index].origin.y) / (ray[ray_index].direction.y);
        coeffs[5] = (_max.z - ray[ray_index].origin.z) / (ray[ray_index].direction.z);

        float t = 0;
        [unroll]
        for (int i = 0; i < 6; ++i) {
            if (coeffs[i] > 0 && inBoxBounds(mesh_node, ray[ray_index].origin + ray[ray_index].direction * coeffs[i])) {
                if (t == 0 || coeffs[i] < t) {
                    t = coeffs[i];
                }
            }
        }

        if (t > 0) {
            return t;
        }
    }

    return 0;
}

float triangleIntersection(in Ray ray[3], float3 v0, float3 v1, float3 v2, float3 normal1, float3 normal2, float3 normal3, inout float3 o_normal)
{
    float3 normal = normalize(cross(normalize(v1 - v0), normalize(v2 - v0)));

    float3 edge1 = v1 - v0;
    float3 edge2 = v2 - v0;
    float3 _u1 = (edge1 * (dot(edge2, edge2)) - edge2 * (dot(edge1, edge2))) / ((dot(edge1, edge1)) * (dot(edge2, edge2)) - (dot(edge1, edge2)) * (dot(edge1, edge2)));
    float3 _v1 = (edge2 * (dot(edge1, edge1)) - edge1 * (dot(edge1, edge2))) / ((dot(edge1, edge1)) * (dot(edge2, edge2)) - (dot(edge1, edge2)) * (dot(edge1, edge2)));
    float u = dot(v0, _u1);
    float v = dot(v0, _v1);

    [unroll]
    for (int i = 0; i < 3; ++i) {
        float dn = dot(ray[i].direction, normal);
        if (dn == 0) {
            return 0;
        }

        float t = -(dot(ray[i].origin, normal) - dot(v0, normal)) / (dn);
        if (t < 0) {
            return 0;
        }

        float3 p = ray[i].origin + ray[i].direction * t;

        float _u = dot(p, _u1) - u;
        if (_u < 0 || _u > 1) {
            return 0;
        }

        float _v = dot(p, _v1) - v;
        if (_v < 0 || _v > 1) {
            return 0;
        }

        if (_u + _v > 1) {
            return 0;
        }

        float _w = (1 - _v - _u);

        o_normal = normal1 * _w + normal2 * _u + normal3 * _v;
        return t;
    }
    return 0;
}

#define BITS_COUNT 32
#define ARRAY_SIZE 64
[numthreads(4, 4, 4)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Voxel currentVoxel = unpack_voxel(VOXELS[uint3(dispatchThreadID.x, dispatchThreadID.y, dispatchThreadID.z)]);
    int skiped = int(currentVoxel.sharpness);
    if (skiped > 0) {
        currentVoxel.sharpness = float(skiped - 1);
        VOXELS[uint3(dispatchThreadID.x, dispatchThreadID.y, dispatchThreadID.z)] = pack_voxel(currentVoxel);
        return;
    }
    uint hit_buffer[ARRAY_SIZE] = (uint[ARRAY_SIZE])0; // [optimization] 32-bit buffer to store box hits

    float t = 0;
    float3 normal = (0).xxx;
    float box_t = 0;
    Ray rays[3] = { GenerateRayForward(dispatchThreadID), GenerateRayRight(dispatchThreadID), GenerateRayUp(dispatchThreadID) };
    for (uint i = 0; i < voxelGrid.mesh_node_count; ++i) {
        uint parent_index = (max(i, 1u) - 1u) / 2u;
        while (parent_index / BITS_COUNT > ARRAY_SIZE) {
            parent_index = (max(parent_index, 1u) - 1u) / 2u;
        }
        uint parent_hit_bit = (1U << (parent_index - (parent_index / BITS_COUNT) * BITS_COUNT));
        bool need_check = (i == 0) || (hit_buffer[parent_index / BITS_COUNT] & parent_hit_bit);

        bool box_intersected = false;
        float box_distance = 0;
        float tri_t = 0;
        float3 tri_normal = (0).xxx;
        { // check box intersection
            box_intersected = false;
            if (!need_check) {
                continue;
            }
            float box_distance = boxIntersection(rays, MESH_TREE[i]);
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
                float _tri_t = triangleIntersection(rays, mul(MODEL_MATRICES[0], float4(VERTICES[INDICES[j + 0]].position, 1.f)).xyz,
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

        uint hit_index = i / BITS_COUNT;
        if (hit_index < ARRAY_SIZE) {
            hit_buffer[hit_index] |= (1U << (i - hit_index * BITS_COUNT));
        }

        if (tri_t > 0 && (tri_t < t || t == 0)) {
            box_t = box_distance;
            t = tri_t;
            normal = tri_normal;
            break;
        }
    }

    Voxel voxel = (Voxel)0;
    if (t > 0) {
        voxel.albedo.x = dispatchThreadID.x;
        voxel.albedo.y = dispatchThreadID.y;
        voxel.albedo.z = dispatchThreadID.z;
        // voxel.normal.x = rays[1].direction.x;
        // voxel.normal.y = rays[1].direction.y;
        // voxel.normal.z = rays[1].direction.z;
        voxel.normal = normal;
        voxel.metalness = t;
        voxel.sharpness = 0;
    } else {
        // between 10 and 30
        uint frames_to_skip = ((dispatchThreadID.x * 900 + dispatchThreadID.y * 140 + dispatchThreadID.z * 4895) % 200) + 100;
        voxel.sharpness = float(frames_to_skip);
    }
    VOXELS[uint3(dispatchThreadID.x, dispatchThreadID.y, dispatchThreadID.z)] = pack_voxel(voxel);
}