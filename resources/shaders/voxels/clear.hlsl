#define CAMERA_REGISTER b0
#define VOXEL_GRID_REGISTER b1
#include "resources/shaders/voxels/voxel.fx"

RWStructuredBuffer<Voxel> voxels : register(u0);

[numthreads(256,1,1)]
void CSMain( uint3 id : SV_DispatchThreadID ) {
    voxels[id.x] = (Voxel) 0;
}
