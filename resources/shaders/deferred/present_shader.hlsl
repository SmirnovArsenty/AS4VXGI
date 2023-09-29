float4 VSMain( unsigned int id : SV_VertexID ) : SV_POSITION
{
    return float4(4 * ((id & 2) >> 1) - 1.0, 4 * (id & 1) - 1.0, 0, 1);
}

Texture2D<float4> position_tex          : register(t0);
Texture2D<float4> normal_tex            : register(t1);
Texture2D<float4> diffuse_tex           : register(t2);
Texture2D<float4> specular_tex          : register(t3);
Texture2D<float4> ambient_tex           : register(t4);
Texture2D<float> depth_tex              : register(t5);
Texture2D<float4> texture_to_present    : register(t6);

float4 PSMain( float4 pos : SV_POSITION ) : SV_Target
{
    // if (depth_tex.Load(int3(pos.xy, 0)).x == 1) {
    //     return float4(.3f, .5f, .7f, 1.f); // sky color
    // }
    return pow(abs(texture_to_present.Load(int3(pos.xy, 0))), 1/2.2f);
}
