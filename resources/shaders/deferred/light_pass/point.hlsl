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
    float4x4 transform;
    float4 light_color;
    float4 light_position_radius;
};

struct PS_IN
{
    float4 position : SV_POSITION;
    // float4 world_position : POSITION;
};

PS_IN VSMain(float3 position : POSITION)
{
    PS_IN res = (PS_IN)0;
    // res.world_position = mul(transform, float4(position, 1.f));
    res.position = mul(view_proj, mul(transform, float4(position, 1.f)));
    return res;
}

Texture2D<float4> position_tex          : register(t0);
Texture2D<float4> normal_tex            : register(t1);
Texture2D<float4> diffuse_tex           : register(t2);
Texture2D<float4> specular_tex          : register(t3);
Texture2D<float4> ambient_tex           : register(t4);
// Texture2D<float> depth_tex              : register(t5);

Texture2D<float> shadow_cascade[CASCADE_COUNT] : register(t6);

float4 PSMain(PS_IN input) : SV_Target
{
    //return float4(1.0f, 1.0f, 1.0f, 1.0f);
    int3 sample_index = int3(input.position.xy, 0);

    float4 result;

    float4 position = position_tex.Load(sample_index);
    float3 to_l = (light_position_radius.xyz - position.xyz);
    float distance = length(to_l);

    if (distance > light_position_radius.w)
        discard;

    float radius = light_position_radius.w;
    float a = 1 - distance / radius;

    float3 light_direction = normalize(to_l);
    float3 normal = normal_tex.Load(sample_index).xyz;

    // float depth = depth_tex.Load(sample_index);

    float4 kd = diffuse_tex.Load(sample_index);
    float4 specular = specular_tex.Load(sample_index);

    float diffuse = max(dot(light_direction, normal), 0.0f) * (a * a);
    float3 reflected_light_dir = reflect(light_direction, normal);
    float cos_phi = dot(reflected_light_dir, normalize(camera_pos - position.xyz));
    result = light_color * kd * diffuse;

    return float4(result.xyz, 1.f);
}
