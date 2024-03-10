#include "voxel.fx"

[numthreads(256,1,1)]
void CSMain( uint3 id : SV_DispatchThreadID ) {
    voxels[id.x] = (Voxel) 0;
}
