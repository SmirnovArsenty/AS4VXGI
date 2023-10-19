#include "framework/shaders/common/translate.fx"

struct CameraData
{
    MATRIX vp;
    MATRIX vp_inverse;
    FLOAT3 position;
    float screen_width;
    FLOAT3 forward;
    float screen_height;
};
