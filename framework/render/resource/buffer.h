#pragma once

#include <d3d11.h>
#include <vector>
#include <string>
#include <cassert>

#include "render/d3d11_common.h"

class Buffer
{
protected:
    ID3D11Buffer* resource_{ nullptr };
    ID3D11ShaderResourceView* resource_view_{ nullptr };
public:
    Buffer() = default;

    ~Buffer()
    {
        assert(resource_ == nullptr);
    }

    inline ID3D11Buffer* const& getBuffer() const
    {
        return resource_;
    }

    inline ID3D11ShaderResourceView* const& getSRV() const
    {
        return resource_view_;
    }

    void set_name(const std::string& name)
    {
        assert(resource_ != nullptr);
        resource_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(name.size()), name.c_str());
    }

    void destroy()
    {
        SAFE_RELEASE(resource_);
        SAFE_RELEASE(resource_view_);
    }
};

template<class T>
class ConstBuffer : public Buffer
{
public:
    ConstBuffer() = default;
    void initialize(T* data)
    {
        D3D11_BUFFER_DESC buffer_desc{};
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buffer_desc.CPUAccessFlags = 0; // don't need CPU write on const buffer
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;
        buffer_desc.ByteWidth = sizeof(T);

        D3D11_SUBRESOURCE_DATA subresource_data;
        subresource_data.pSysMem = data;
        subresource_data.SysMemPitch = 0;
        subresource_data.SysMemSlicePitch = 0;

        auto device = Game::inst()->render().device();
        D3D11_CHECK(device->CreateBuffer(&buffer_desc_, &subresource_data, &resource_));
    }
};

template<class T>
class DynamicBuffer : public Buffer
{
public:
    DynamicBuffer() = default;

    void initialize(T* data)
    {
        D3D11_BUFFER_DESC buffer_desc{};
        buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
        buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;
        buffer_desc.ByteWidth = sizeof(T);

        D3D11_SUBRESOURCE_DATA subresource_data;
        subresource_data.pSysMem = data;
        subresource_data.SysMemPitch = 0;
        subresource_data.SysMemSlicePitch = 0;

        auto device = Game::inst()->render().device();
        D3D11_CHECK(device->CreateBuffer(&buffer_desc, &subresource_data, &resource_));
    }

    void update(T* data)
    {
        auto context = Game::inst()->render().context();
        D3D11_MAPPED_SUBRESOURCE mss;
        context->Map(resource_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mss);
        memcpy(mss.pData, data, sizeof(T));
        context->Unmap(resource_, 0);
    }
};

template<class T>
class ShaderResource : public Buffer
{
public:
    ShaderResource() = default;

    void initialize(T* data, uint32_t count)
    {
        D3D11_BUFFER_DESC buffer_desc;
        buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
        buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        buffer_desc.StructureByteStride = sizeof(T);
        buffer_desc.ByteWidth = sizeof(T) * count;

        D3D11_SUBRESOURCE_DATA subresource_data;
        subresource_data.pSysMem = data;
        subresource_data.SysMemPitch = 0;
        subresource_data.SysMemSlicePitch = 0;

        auto device = Game::inst()->render().device();
        D3D11_CHECK(device->CreateBuffer(&buffer_desc, &subresource_data, &resource_));

        // create SRV
        D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc;
        resource_view_desc.BufferEx.FirstElement = 0;
        resource_view_desc.BufferEx.NumElements = count;
        resource_view_desc.BufferEx.Flags = 0;
        resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
        D3D11_CHECK(device->CreateShaderResourceView(resource_, &resource_view_desc, &resource_view_));
    }
};

template<class T>
class IndexBuffer : public Buffer
{
public:
    const T buffer_type;

    IndexBuffer()
    {
        static_assert(std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>());
    };

    void initialize(T* data, uint32_t count)
    {
        D3D11_BUFFER_DESC buffer_desc;
        buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
        buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;
        buffer_desc.ByteWidth = count;

        D3D11_SUBRESOURCE_DATA subresource_data;
        subresource_data.pSysMem = data;
        subresource_data.SysMemPitch = 0;
        subresource_data.SysMemSlicePitch = 0;

        auto device = Game::inst()->render().device();
        D3D11_CHECK(device->CreateBuffer(&buffer_desc, &subresource_data, &resource_));
    }

    void initialize(T* data, size_t count)
    {
        initialize(data, static_cast<uint32_t>(count));
    }

    void bind()
    {
        auto context = Game::inst()->render().context();
        if (std::is_same<T, uint16_t>()) {
            context->IASetIndexBuffer(resource_, DXGI_FORMAT_R16_UINT, 0);
        } else if (std::is_same<T, uint32_t>()) {
            context->IASetIndexBuffer(resource_, DXGI_FORMAT_R32_UINT, 0);
        }
    }
};

template<class T>
class VertexBuffer : public Buffer
{
public:
    const T buffer_type;

    VertexBuffer() = default;

    void initialize(T* data, uint32_t count)
    {
        D3D11_BUFFER_DESC buffer_desc;
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_desc.CPUAccessFlags = 0;
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;
        buffer_desc.ByteWidth = sizeof(T) * count;

        D3D11_SUBRESOURCE_DATA subresource_data;
        subresource_data.pSysMem = data;
        subresource_data.SysMemPitch = 0;
        subresource_data.SysMemSlicePitch = 0;

        auto device = Game::inst()->render().device();
        D3D11_CHECK(device->CreateBuffer(&buffer_desc, &subresource_data, &resource_));
    }

    void initialize(T* data, size_t count)
    {
        initialize(data, static_cast<uint32_t>(count));
    }

    void bind(UINT slot)
    {
        auto context = Game::inst()->render().context();
        UINT strides{ sizeof(T) };
        UINT offsets{ 0 };
        context->IASetVertexBuffers(slot, 1U, &resource_, &strides, &offsets);
    }
};
