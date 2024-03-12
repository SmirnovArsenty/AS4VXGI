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
// UINT is known
struct CBV {};
struct SRV {};
struct UAV {};
template<class t, UINT sl, UINT sp>
struct BindInfo
{
    t type() {return t{}};
    UINT slot() {return sl;};
    UINT space() {return sp;};
};
#define DECLARE_CBV(NAME, SLOT, SPACE) struct NAME : public BindInfo<CBV, SLOT, SPACE>
#define DECLARE_SRV(NAME, SLOT, SPACE) struct NAME : public BindInfo<SRV, SLOT, SPACE> {};
#define DECLARE_UAV(NAME, TYPE, SLOT, SPACE) struct NAME : public BindInfo<UAV, SLOT, SPACE> {};

#else

#define FLOAT2 float2
#define FLOAT3 float3
#define FLOAT4 float4
#define MATRIX float4x4
#define UINT uint

#define DECLARE_CBV(NAME, SLOT, SPACE) cbuffer NAME : register(b##SLOT, space##SPACE)
#define DECLARE_SRV(NAME, SLOT, SPACE) Texture2D NAME : register(t##SLOT, space##SPACE);
#define DECLARE_UAV(NAME, TYPE, SLOT, SPACE) RWStructuredBuffer<TYPE> NAME : register(u##SLOT, space##SPACE);

#endif

#endif //__TRANSLATE_FX__
