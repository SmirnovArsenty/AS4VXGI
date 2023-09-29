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
    float3 light_direction;
    float dummy_0;
    float3 light_color;
    float dummy_1;
};

Texture2D<float4> position_tex          : register(t0);
Texture2D<float4> normal_tex            : register(t1);
Texture2D<float4> diffuse_tex           : register(t2);
Texture2D<float4> specular_tex          : register(t3);
Texture2D<float4> ambient_tex           : register(t4);
// Texture2D<float> depth_tex              : register(t5);

Texture2D<float> shadow_cascade[CASCADE_COUNT] : register(t6);

// SamplerComparisonState depth_sampler    : register(s1);

float4 PSMain(float4 position : SV_POSITION) : SV_Target0
{
    float3 result;

    float3 normal = normal_tex.Load(int3(position.xy, 0)).xyz;
    float cos_ln = dot(normal, normalize(-light_direction));
    if (cos_ln < 0) {
        return (0).xxxx;
    }

    // float depth = depth_tex.Load(int3(position.xy, 0));
    float4 pos = position_tex.Load(int3(position.xy, 0));

    float4 diffuse = diffuse_tex.Load(int3(position.xy, 0));
    float4 specular = specular_tex.Load(int3(position.xy, 0));

    float3 reflected_light_dir = reflect(normalize(light_direction), normal);
    float cos_phi = dot(reflected_light_dir, normalize(camera_pos - pos.xyz));
    result = 
        dot(normal, -light_direction) * diffuse.xyz * light_color +
        light_color * specular.xyz * pow(max(cos_phi, 0), 20);

    return float4(result, 1.f);
}
