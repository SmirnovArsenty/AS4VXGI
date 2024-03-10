#ifndef __VOXEL_FX__
#define __VOXEL_FX__

#include "../../../framework/shaders/common/types.fx"

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

    float unit = voxelGrid.size / voxelGrid.dimension;

    float3 up = float3(0, 1, 0);
    float3 right = cross(cameraData.forward, up);
    right = normalize(right);
    up = cross(cameraData.forward, right);
    up = normalize(up);

    res.origin = cameraData.position;
    res.origin -= cameraData.forward * (voxelGrid.size / 2);
    res.origin -= right * (voxelGrid.size / 2);
    res.origin -= up * (voxelGrid.size / 2);

    // + 0.5 to place ray origin in the center of voxel
    res.origin += unit * (voxel_location.x * right);
    res.origin += unit * (voxel_location.y * up + 0.5);
    res.origin += unit * (voxel_location.z * cameraData.forward - 0.5);

    res.direction = normalize(cameraData.forward);
    //res.direction = ((uint)(voxel_location.z * 2) < voxel_grid.dimension) ? -res.direction : res.direction;

    return res;
}

Ray GenerateRayRight(uint3 voxel_location)
{
    Ray res;

    float unit = voxelGrid.size / voxelGrid.dimension;

    float3 up = float3(0, 1, 0);
    float3 right = cross(cameraData.forward, up);
    right = normalize(right);
    up = cross(cameraData.forward, right);
    up = normalize(up);

    res.origin = cameraData.position;
    res.origin -= cameraData.forward * (voxelGrid.size / 2);
    res.origin -= right * (voxelGrid.size / 2);
    res.origin -= up * (voxelGrid.size / 2);

    // + 0.5 to place ray origin in the center of voxel
    res.origin += unit * (voxel_location.x * right);
    res.origin += unit * (voxel_location.y * up + 0.5);
    res.origin += unit * (voxel_location.z * cameraData.forward + 0.5);

    res.direction = right;
    //res.direction = ((uint)(voxel_location.x * 2) < voxel_grid.dimension) ? -res.direction : res.direction;

    return res;
}

Ray GenerateRayUp(uint3 voxel_location)
{
    Ray res;

    float unit = voxelGrid.size / voxelGrid.dimension;

    float3 up = float3(0, 1, 0);
    float3 right = cross(cameraData.forward, up);
    right = normalize(right);
    up = cross(cameraData.forward, right);
    up = normalize(up);

    res.origin = cameraData.position;
    res.origin -= cameraData.forward * (voxelGrid.size / 2);
    res.origin -= right * (voxelGrid.size / 2);
    res.origin -= up * (voxelGrid.size / 2);

    // + 0.5 to place ray origin in the center of voxel
    res.origin += unit * (voxel_location.x * right + 0.5);
    res.origin += unit * (voxel_location.y * up);
    res.origin += unit * (voxel_location.z * cameraData.forward + 0.5);

    res.direction = up;
    //res.direction = ((uint)(voxel_location.y * 2) < voxel_grid.dimension) ? -res.direction : res.direction;

    return res;
}

#endif

#endif // __VOXEL_FX__
