#ifndef __TRANSLATE_FX__
#define __TRANSLATE_FX__

#if defined(__cplusplus)

#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

struct DummyStructure{};

#define FLOAT2 Vector2
#define FLOAT3 Vector3
#define FLOAT4 Vector4
#define MATRIX Matrix
#define cbuffer struct

#else

#define FLOAT2 float2
#define FLOAT3 float3
#define FLOAT4 float4
#define MATRIX float4x4

#endif

#endif //__TRANSLATE_FX__
