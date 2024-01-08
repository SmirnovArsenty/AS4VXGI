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
    UINT mesh_node_count;
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

Ray GenerateRayForward(uint3 voxel_location)
{
    Ray res;

    float unit = voxel_grid.size / voxel_grid.dimension;

    float3 up = float3(0, 1, 0);
    float3 right = cross(camera.forward, up);
    right = normalize(right);
    up = cross(camera.forward, right);
    up = normalize(up);

    res.origin = camera.position;
    res.origin -= camera.forward * (voxel_grid.size / 2);
    res.origin -= right * (voxel_grid.size / 2);
    res.origin -= up * (voxel_grid.size / 2);

    // + 0.5 to place ray origin in the center of voxel
    res.origin += unit * (voxel_location.x * right);
    res.origin += unit * (voxel_location.y * up + 0.5);
    res.origin += unit * (voxel_location.z * camera.forward - 0.5);

    res.direction = normalize(camera.forward);
    //res.direction = ((uint)(voxel_location.z * 2) < voxel_grid.dimension) ? -res.direction : res.direction;

    return res;
}

Ray GenerateRayRight(uint3 voxel_location)
{
    Ray res;

    float unit = voxel_grid.size / voxel_grid.dimension;

    float3 up = float3(0, 1, 0);
    float3 right = cross(camera.forward, up);
    right = normalize(right);
    up = cross(camera.forward, right);
    up = normalize(up);

    res.origin = camera.position;
    res.origin -= camera.forward * (voxel_grid.size / 2);
    res.origin -= right * (voxel_grid.size / 2);
    res.origin -= up * (voxel_grid.size / 2);

    // + 0.5 to place ray origin in the center of voxel
    res.origin += unit * (voxel_location.x * right);
    res.origin += unit * (voxel_location.y * up + 0.5);
    res.origin += unit * (voxel_location.z * camera.forward + 0.5);

    res.direction = right;
    //res.direction = ((uint)(voxel_location.x * 2) < voxel_grid.dimension) ? -res.direction : res.direction;

    return res;
}

Ray GenerateRayUp(uint3 voxel_location)
{
    Ray res;

    float unit = voxel_grid.size / voxel_grid.dimension;

    float3 up = float3(0, 1, 0);
    float3 right = cross(camera.forward, up);
    right = normalize(right);
    up = cross(camera.forward, right);
    up = normalize(up);

    res.origin = camera.position;
    res.origin -= camera.forward * (voxel_grid.size / 2);
    res.origin -= right * (voxel_grid.size / 2);
    res.origin -= up * (voxel_grid.size / 2);

    // + 0.5 to place ray origin in the center of voxel
    res.origin += unit * (voxel_location.x * right + 0.5);
    res.origin += unit * (voxel_location.y * up);
    res.origin += unit * (voxel_location.z * camera.forward + 0.5);

    res.direction = up;
    //res.direction = ((uint)(voxel_location.y * 2) < voxel_grid.dimension) ? -res.direction : res.direction;

    return res;
}

#endif

#endif // __VOXEL_FX__
