#pragma once

#include <string>
#include <dxgiformat.h>
#include <d3d11.h>

class Texture
{
public:
    Texture();
    ~Texture();

    void load(const std::string& path);
    void initialize(uint32_t width, uint32_t height, DXGI_FORMAT format, void* pixel_data, D3D11_BIND_FLAG bind_flag = D3D11_BIND_SHADER_RESOURCE);
    void destroy();

    void bind(UINT slot);

    ID3D11Resource* resource() const;
    ID3D11ShaderResourceView* view() const;

private:
    ID3D11Texture2D* texture_{ nullptr };
    ID3D11ShaderResourceView* resource_view_{ nullptr };

    void* own_pixel_data_{ nullptr };
};
