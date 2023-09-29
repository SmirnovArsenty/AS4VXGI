struct VS_IN
{
    float4 pos_uv_x : POSITION_UV_X0;
    float4 normal_uv_y : NORMAL_UV_Y0;
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
    float4 position : SV_Target0;
    float4 normal   : SV_Target1;
    float4 diffuse  : SV_Target2;
    float4 specular : SV_Target3;
    float4 ambient  : SV_Target4;
};

cbuffer SceneData : register(b0)
{
    float4x4 view_proj;
    float4x4 inv_view_proj;
    float3 camera_pos;
    float screen_width;
    float3 camera_dir;
    float screen_heght;
};

cbuffer ModelData : register(b1)
{
    float4x4 transform;
    float4x4 inverse_transpose_transform;
};

cbuffer MeshData : register(b2)
{
    uint is_pbr;
    uint material_flags;
    float2 MeshData_dummy;
};

Texture2D<float4> diffuse_tex   : register(t1);
Texture2D<float4> specular_tex  : register(t2);
Texture2D<float4> ambient_tex   : register(t3);
SamplerState tex_sampler : register(s0);

PS_IN VSMain(VS_IN input)
{
    PS_IN res = (PS_IN)0;
    res.world_model_pos = mul(transform, float4(input.pos_uv_x.xyz, 1.f));
    res.pos = mul(view_proj, float4(res.world_model_pos.xyz, 1.f));
    res.normal = mul(inverse_transpose_transform, float4(input.normal_uv_y.xyz, 0.f));
    res.uv = float2(input.pos_uv_x.w, input.normal_uv_y.w);

    return res;
}

[earlydepthstencil]
PS_OUT PSMain(PS_IN input)
{
    PS_OUT res = (PS_OUT)0;

    res.position = input.world_model_pos;

    // float3 normal = normalize(input.normal.xyz);
    // res.normal = float4(normal * (0.5).xxx + (0.5).xxx, 1.f);
    res.normal = float4(normalize(input.normal.xyz), 1.f);

    { // Phong light model
        float4 diffuse_color = (0).xxxx;
        float4 specular_color = (0).xxxx;
        float4 ambient_color = (0).xxxx;

        if (material_flags & 1) {
            diffuse_color = diffuse_tex.Sample(tex_sampler, input.uv);
            diffuse_color = pow(abs(diffuse_color), 2.2f);
        }
        if (material_flags & 2) {
            specular_color = specular_tex.Sample(tex_sampler, input.uv);
        }
        if (material_flags & 4) {
            ambient_color = ambient_tex.Sample(tex_sampler, input.uv);
        }

        if ((material_flags & 7) == 0) { // no material provided - draw gray
            diffuse_color = float4(.2f, .2f, .2f, 1.f);
            specular_color = (0.5).xxxx;
            ambient_color = (0.1).xxxx;
        }

        res.diffuse = diffuse_color;
        res.specular = specular_color;
        res.ambient = ambient_color;
    }

    return res;
}
