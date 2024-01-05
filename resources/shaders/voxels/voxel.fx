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

#endif // __VOXEL_FX__
