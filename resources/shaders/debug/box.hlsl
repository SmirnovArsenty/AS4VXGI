#include "framework/shaders/common/registers.hlsl"

struct VS_IN
{
    float4 position : POSITION0;
    uint index : SV_InstanceID;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 world_model_pos : POSITION0;
};

struct PS_OUT
{
    float4 color : SV_Target0;
};

cbuffer ModelData : register(b1)
{
    float4x4 transform;
    float4x4 inverse_transpose_transform;
};

StructuredBuffer<float4x4> box_transform : register(t0);

PS_IN VSMain(VS_IN input)
{
    PS_IN res = (PS_IN)0;

    res.pos = mul(camera.vp, float4(mul(box_transform[input.index], mul(transform, float4(input.position))).xyz, 1.f));

    return res;
}

[earlydepthstencil]
PS_OUT PSMain(PS_IN input)
{
    PS_OUT res = (PS_OUT)0;
    res.color = float4(0.8, 0.8, 0.8, 1.0);
    return res;
}
