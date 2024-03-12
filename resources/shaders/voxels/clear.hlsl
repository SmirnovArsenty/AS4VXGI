#include "voxel.fx"

[numthreads(256,1,1)]
void CSMain( uint3 id : SV_DispatchThreadID ) {
    VOXELS[id.x] = (Voxel) 0;
}
