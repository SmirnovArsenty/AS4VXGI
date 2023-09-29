#include <cassert>

#include <WICTextureLoader.h>
using namespace DirectX;

#include "core/game.h"
#include "render/render.h"
#include "render/d3d11_common.h"
#include "texture.h"

Texture::Texture()
{
}

Texture::~Texture()
{
    destroy(); // for default static material texture
}

void Texture::load(const std::string& path)
{
    // load texture color
    assert(!path.empty());
    assert(texture_ == nullptr);
    auto device = Game::inst()->render().device();
    auto context = Game::inst()->render().context();
    std::wstring filenamew(path.begin(), path.end());
    CreateWICTextureFromFile(device, filenamew.c_str(), (ID3D11Resource**)&texture_, &resource_view_);

    assert(texture_ != nullptr);
    assert(resource_view_ != nullptr);
}

void Texture::initialize(uint32_t width, uint32_t height, DXGI_FORMAT format, void* pixel_data, D3D11_BIND_FLAG bind_flag)
{
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Format = format;
    desc.BindFlags = bind_flag;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA subresourceData;
    if (pixel_data == nullptr)
    {
        size_t pixel_data_size = width * height;
        if (format == DXGI_FORMAT_B8G8R8A8_UNORM ||
            format == DXGI_FORMAT_R8G8B8A8_UNORM ||
            format == DXGI_FORMAT_D32_FLOAT ||
            format == DXGI_FORMAT_R32_TYPELESS ||
            format == DXGI_FORMAT_R32_FLOAT)
        {
            pixel_data_size *= 4;
        }
        own_pixel_data_ = new char[pixel_data_size];
        pixel_data = own_pixel_data_;
    }
    subresourceData.pSysMem = pixel_data;
    if (format == DXGI_FORMAT_B8G8R8A8_UNORM ||
        format == DXGI_FORMAT_R8G8B8A8_UNORM ||
        format == DXGI_FORMAT_D32_FLOAT ||
        format == DXGI_FORMAT_R32_TYPELESS ||
        format == DXGI_FORMAT_R32_FLOAT) {
        subresourceData.SysMemPitch = width * 4;
    } else {
        assert(false);
    }
    subresourceData.SysMemSlicePitch = width * height * 4;

    auto device = Game::inst()->render().device();
    D3D11_CHECK(device->CreateTexture2D(&desc, &subresourceData, &texture_));
    assert(texture_ != nullptr);

    if (!(bind_flag & (D3D11_BIND_RENDER_TARGET))) {
        if (format == DXGI_FORMAT_R32_TYPELESS)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
            srv_desc.Texture2D.MipLevels = 1;
            D3D11_CHECK(device->CreateShaderResourceView(texture_, &srv_desc, &resource_view_));
        }
        else
        {
            D3D11_CHECK(device->CreateShaderResourceView(texture_, nullptr, &resource_view_));
        }
        assert(resource_view_ != nullptr);
    }
}

void Texture::destroy()
{
    if (texture_ != nullptr) {
        texture_->Release();
        texture_ = nullptr;
    }

    if (resource_view_ != nullptr) {
        resource_view_->Release();
        resource_view_ = nullptr;
    }

    if (own_pixel_data_ != nullptr) {
        delete[] own_pixel_data_;
        own_pixel_data_ = nullptr;
    }
}

void Texture::bind(UINT slot)
{
    auto context = Game::inst()->render().context();
    context->PSSetShaderResources(slot, 1, &resource_view_);
}

ID3D11Resource* Texture::resource() const
{
    return (ID3D11Resource*)texture_;
}

ID3D11ShaderResourceView* Texture::view() const
{
    return resource_view_;
}
