#include "../../../framework/shaders/common/types.fx"

struct VS_IN
{
    float3 position : POSITION0;
    float texcoord_u : TEXCOORD_U0;
    float texcoord_v : TEXCOORD_V0;
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

Texture2D<float4> albedo_tex : register(t0);
SamplerState tex_sampler : register(s0);

PS_IN VSMain(VS_IN input)
{
    PS_IN res = (PS_IN)0;

    res.world_model_pos = mul(transform, float4(input.position, 1.f));
    res.pos = mul(camera.vp, float4(res.world_model_pos.xyz, 1.f));
    res.normal = mul(inverse_transpose_transform, float4(input.normal, 0.f));
    res.uv = float2(input.texcoord_u, input.texcoord_v);

    return res;
}

[earlydepthstencil]
PS_OUT PSMain(PS_IN input)
{
    PS_OUT res = (PS_OUT)0;

    float4 albedo_color;
    albedo_color = albedo_tex.Sample(tex_sampler, input.uv);
    res.color = pow(abs(albedo_color), 2.2f);

    if (res.color.x == 0 && res.color.y == 0 && res.color.z == 0 && res.color.w == 0) {
        res.color = (0.2).xxxx;
    }

    return res;
}
