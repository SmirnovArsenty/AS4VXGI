#ifndef __TYPES_FX__
#define __TYPES_FX__

#include "framework/shaders/common/translate.fx"

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

#ifndef __cplusplus

#ifndef CAMERA_REGISTER
#error CAMERA_REGISTER is undefined
#endif

cbuffer __CameraData__ : register(CAMERA_REGISTER)
{
    CameraData camera;
}

#endif

#endif // __TYPES_FX__
