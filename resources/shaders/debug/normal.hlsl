#define CAMERA_REGISTER b0
#include "../../../framework/shaders/common/types.fx"

struct VS_IN
{
    float3 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 world_model_pos : POSITION0;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct PS_OUT
{
    float4 color : SV_Target0;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN res = (PS_IN)0;

    res.world_model_pos = mul(transform, float4(input.position, 1.f));
    res.pos = mul(cameraData.vp, float4(res.world_model_pos.xyz, 1.f));
    res.normal = mul(inverse_transpose_transform, float4(input.normal, 0.f));
    res.uv = input.texcoord;

    return res;
}

[earlydepthstencil]
PS_OUT PSMain(PS_IN input)
{
    PS_OUT res = (PS_OUT)0;

    res.color = input.normal * 0.2;

    return res;
}
