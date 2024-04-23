#ifndef __TYPES_FX__
#define __TYPES_FX__

#include "translate.fx"

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
    FLOAT4 _[14]; // align by D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT (256)
};

struct VoxelGrid
{
    int dimension;
    float size;
    UINT mesh_node_count;
    float dummy;

    FLOAT4 _[15]; // align by D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT (256)
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

// space 0
DECLARE_CBV(CAMERA_DATA, 0, 0)
{
    CameraData cameraData;
};
DECLARE_CBV(MODEL_DATA, 1, 0)
{
    MATRIX transform;
    MATRIX inverse_transpose_transform;
    MATRIX dummy[2];
};
DECLARE_SRV(MESH_TREE, MeshTreeNode, 2, 0)
DECLARE_SRV(INDICES, int, 3, 0)
DECLARE_SRV(VERTICES, Vertex, 4, 0)
DECLARE_SRV(MODEL_MATRICES, MATRIX, 5, 0)
DECLARE_SRV(BOX_TRANSFORM, MATRIX, 6, 0)

// space 1
DECLARE_CBV(VOXEL_DATA, 0, 1)
{
    VoxelGrid voxelGrid;
};
DECLARE_UAV(VOXELS, float4, 1, 1)

#endif // __TYPES_FX__
