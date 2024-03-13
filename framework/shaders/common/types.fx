#ifndef __TYPES_FX__
#define __TYPES_FX__

#include "../../../framework/shaders/common/translate.fx"

struct CameraData
{
    MATRIX vp;
    MATRIX vp_inverse;
    MATRIX projection; // camera space
    MATRIX projection_inverse; // camera space
    FLOAT3 position;
    float screen_width;
    FLOAT3 forward;
    float screen_height;
};

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

DECLARE_CBV(COMMON, 0, 0)
{
    CameraData cameraData;
    VoxelGrid voxelGrid;
};

DECLARE_UAV(VOXELS, Voxel, 0, 0)

#endif // __TYPES_FX__
