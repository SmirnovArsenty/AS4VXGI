float4 VSMain( unsigned int id : SV_VertexID ) : SV_POSITION
{
    return float4(4 * ((id & 2) >> 1) - 1.0, 4 * (id & 1) - 1.0, 0, 1);
}

cbuffer SceneData : register(b0)
{
    float4x4 view_proj;
    float4x4 inv_view_proj;
    float3 camera_pos;
    float screen_width;
    float3 camera_dir;
    float screen_heght;
};

cbuffer LightData : register(b1)
{
    float3 color;
};

Texture2D<float4> position_tex          : register(t0);
Texture2D<float4> normal_tex            : register(t1);
Texture2D<float4> diffuse_tex           : register(t2);
Texture2D<float4> specular_tex          : register(t3);
Texture2D<float4> ambient_tex           : register(t4);
// Texture2D<float> depth_tex              : register(t5);

float4 PSMain(float4 position : SV_POSITION) : SV_Target
{
    float3 diffuse = diffuse_tex.Load(float3(position.xy, 0)).xyz;
    return float4(diffuse * color, 1.f);
}
