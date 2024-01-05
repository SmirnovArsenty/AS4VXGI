#pragma once

#include <d3d11.h>

#include "resource.h"
#include "render/d3d11_common.h"

class Sampler : public Resource
{
private:
    ID3D11SamplerState* sampler_state_{ nullptr };
public:
    Sampler() = default;
    virtual ~Sampler()
    {
        assert(sampler_state_ == nullptr);
    }

    void initialize();

    void destroy() override
    {
        SAFE_RELEASE(sampler_state_);
        if (sampler_state_ != nullptr) {
            sampler_state_->Release();
            sampler_state_ = nullptr;
        }
    }

    void use(UINT slot);

    void set_name(const std::string& name) override
    {
        assert(sampler_state_ != nullptr);
        sampler_state_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(name.size()), name.c_str());
    }
};
