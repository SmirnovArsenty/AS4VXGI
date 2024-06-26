#define CAMERA_REGISTER b0
#include "../../../framework/shaders/common/types.fx"

struct VS_IN
{
    uint index : SV_InstanceID;
    float4 position : POSITION0;
};

struct PS_VS
{
    float4 pos : SV_POSITION;
    float4 world_model_pos : POSITION0;
    float4 color : COLOR0;
};

struct PS_OUT
{
    float4 color : SV_Target0;
};

PS_VS VSMain(VS_IN input)
{
    PS_VS res = (PS_VS)0;

    res.pos = mul(cameraData.vp, mul(transform, mul(BOX_TRANSFORM[input.index], input.position)));
    res.color = float4(((255.f - input.index) / 255.f).xxx, 1.f);

    return res;
}

[earlydepthstencil]
PS_OUT PSMain(PS_VS input)
{
    PS_OUT res = (PS_OUT)0;
    res.color = input.color;
    return res;
}
