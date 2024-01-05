#include "sampler.h"
#include "core/game.h"
#include "render/render.h"

void Sampler::initialize()
{
    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    auto device = Game::inst()->render().device();
    D3D11_CHECK(device->CreateSamplerState(&sampler_desc, &sampler_state_));
}

void Sampler::use(UINT slot)
{
    auto context = Game::inst()->render().context();
    context->VSSetSamplers(slot, 1, &sampler_state_);
    context->PSSetSamplers(slot, 1, &sampler_state_);
    context->GSSetSamplers(slot, 1, &sampler_state_);
    context->HSSetSamplers(slot, 1, &sampler_state_);
    context->DSSetSamplers(slot, 1, &sampler_state_);
    context->CSSetSamplers(slot, 1, &sampler_state_);
}
