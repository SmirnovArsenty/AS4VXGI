#pragma once

#include <d3d11_3.h>

#include "resource.h"
#include "render/d3d11_common.h"

class Fence : public Resource
{
private:
    ID3D11Fence* fence_{ nullptr };
public:
    Fence();
    ~Fence() { assert(fence_ == nullptr); }

    void destroy() override
    {
        SAFE_RELEASE(fence_);
    }
    void set_name(const std::string& name)
    {
        assert(fence_ != nullptr);
        fence_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(name.size()), name.c_str());
    }

    void initialize();
};
