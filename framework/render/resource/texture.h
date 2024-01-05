#pragma once

#include <string>
#include <dxgiformat.h>
#include <d3d11.h>
#include <cassert>

#include "resource.h"

class Texture2D : public Resource
{
public:
    Texture2D();
    ~Texture2D();

    void destroy() override;
    void set_name(const std::string& name) override
    {
        assert(texture_ != nullptr);
        texture_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(name.size()), name.c_str());
    }

    void load(const std::string& path);
    void initialize(uint32_t width, uint32_t height, DXGI_FORMAT format, void* pixel_data = nullptr, D3D11_BIND_FLAG bind_flag = D3D11_BIND_SHADER_RESOURCE);

    void bind(UINT slot);

    ID3D11Resource*const& resource() const;
    ID3D11ShaderResourceView*const& view() const;

private:
    ID3D11Texture2D* texture_{ nullptr };
    ID3D11ShaderResourceView* resource_view_{ nullptr };

    void* own_pixel_data_{ nullptr };
};
