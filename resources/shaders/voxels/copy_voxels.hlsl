#include "voxel.fx"

[numthreads(256,1,1)]
void CSMain( uint3 id : SV_DispatchThreadID ) {
    VOXELS_DST[id.x] = VOXELS_SRC[id.x];
}
