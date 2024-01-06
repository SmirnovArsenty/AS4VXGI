#ifndef __VOXEL_FX__
#define __VOXEL_FX__

#include "framework/shaders/common/types.fx"

struct Voxel
{
    FLOAT3 normal;
    float sharpness;
    FLOAT3 albedo;
    float metalness;
};

struct Vertex
{
    FLOAT3 position;
    float uv_x;
    float uv_y;
    FLOAT3 normal;
};

struct MeshTreeNode
{
    FLOAT3 min;
    int start_index;
    FLOAT3 max;
    int count;
};

struct VoxelGrid
{
    int dimension;
    float size;
    int mesh_node_count;
    float dummy;
};

#ifndef __cplusplus

#ifndef VOXEL_GRID_REGISTER
#error VOXEL_GRID_REGISTER is undefined
#endif

cbuffer __VoxelGrid__ : register(VOXEL_GRID_REGISTER)
{
    VoxelGrid voxel_grid;
};

#endif

#ifndef __cplusplus

/////
// common hlsl structures and methods
/////

struct Ray
{
    float3 origin;
    float3 direction;
};

Ray GenerateRay(uint3 voxel_location)
{
    Ray res;
    res.direction = normalize(camera.forward);
    res.origin = camera.position;
    res.origin -= camera.forward * (voxel_grid.size / 2);

    float3 up = float3(0, 1, 0);
    float3 right = cross(camera.forward, up);
    right = normalize(right);
    res.origin -= right * (voxel_grid.size / 2);

    up = cross(right, camera.forward);
    up = normalize(up);
    res.origin -= up * voxel_grid.size / 2;

    float unit = voxel_grid.size / voxel_grid.dimension;
    // + 0.5 to place ray origin in the center of voxel
    res.origin += unit * (voxel_location.x * right + 0.5);
    res.origin += unit * (voxel_location.y * up + 0.5);
    res.origin += unit * (voxel_location.z * camera.forward);

    return res;
}

#endif

#endif // __VOXEL_FX__
