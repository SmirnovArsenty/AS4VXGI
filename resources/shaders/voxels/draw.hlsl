#define CAMERA_REGISTER b0
#define VOXEL_GRID_REGISTER b1
#include "resources/shaders/voxels/voxel.fx"

struct VS_IN
{
    uint index : SV_InstanceID;
};

struct PS_VS
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
};

struct PS_OUT
{
    float4 color : SV_Target0;
};

StructuredBuffer<Voxel> voxels : register(t0);

PS_VS VSMain(VS_IN input)
{
    PS_VS res = (PS_VS)0;

#if 0
    float3 up = float3(0, 1, 0);
    float3 right = cross(camera.forward, up);
    right = normalize(right);
    up = cross(camera.forward, right);
    up = normalize(up);
    float3 forward = camera.forward;
#else
    float3 forward = float3(0, 0, -1);
    float3 up = float3(0, 1, 0);
    float3 right = float3(1, 0, 0);
#endif


    int z_index = input.index / (voxel_grid.dimension * voxel_grid.dimension);
    int y_index = (input.index - z_index * voxel_grid.dimension * voxel_grid.dimension) / voxel_grid.dimension;
    int x_index = input.index - y_index * voxel_grid.dimension - z_index * voxel_grid.dimension * voxel_grid.dimension;

    float unit = voxel_grid.size / voxel_grid.dimension;

    float3 pos = (float3)0;
    pos += right *   (-voxel_grid.size / 2 + unit * x_index - unit * 0.5);
    pos -= up *      (-voxel_grid.size / 2 + unit * y_index);
    pos += forward * (-voxel_grid.size / 2 + unit * z_index);

    res.pos = float4(pos, 1.f);
    //res.pos = mul(camera.vp, float4(pos, 1.f));
    //float3 color = abs(normalize(pos - camera.position));
    float3 color = abs(voxels[input.index].normal);
    res.color = float4(color, 1.f);

    return res;
}

// 0 - boxes
// 1 - lines
#define DRAW_LINES 1
#if DRAW_LINES
[maxvertexcount(18)]
void GSMain(point PS_VS input[1], inout LineStream<PS_VS> OutputStream)
#else
[maxvertexcount(36)]
void GSMain(point PS_VS input[1], inout TriangleStream<PS_VS> OutputStream)
#endif
{
    const float size = (voxel_grid.size / voxel_grid.dimension);

    PS_VS data = input[0];

    if (dot(data.color.xyz, data.color.xyz) == 0) {
        return;
    }

    bool draw_line = false;
    bool draw_cube = true;

    float4x4 transform = camera.projection;
    // transform = camera.vp; // debug

#if DRAW_LINES
    // lines
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();
#else
    // boxes
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + 0, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();

    data.pos = mul(transform, float4(input[0].pos.x + 0, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + size, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    data.pos = mul(transform, float4(input[0].pos.x + size, input[0].pos.y + 0, input[0].pos.z + size, input[0].pos.w));
    OutputStream.Append(data);
    OutputStream.RestartStrip();
#endif
}

[earlydepthstencil]
PS_OUT PSMain(PS_VS input)
{
    PS_OUT res = (PS_OUT)0;
    res.color = input.color;
    return res;
}
