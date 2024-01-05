#pragma once

#include <d3d11.h>
#include <vector>
#include <string>
#include <cstdint>
#include <cassert>

#include "core/game.h"
#include "render/render.h"
#include "resource.h"
#include "render/d3d11_common.h"

class Buffer : public Resource
{
protected:
    ID3D11Buffer* resource_{ nullptr };
    ID3D11ShaderResourceView* resource_view_{ nullptr };
    ID3D11UnorderedAccessView* unordered_access_view_{ nullptr };
public:
    Buffer() = default;

    virtual ~Buffer()
    {
        assert(resource_ == nullptr);
        assert(resource_view_ == nullptr);
        assert(unordered_access_view_ == nullptr);
    }

    inline ID3D11Buffer* const& getBuffer() const { return resource_; }

    inline ID3D11ShaderResourceView*const& getSRV() const { return resource_view_; }

    inline ID3D11UnorderedAccessView*const& getUAV() const { return unordered_access_view_; }

    void set_name(const std::string& name)
    {
        assert(resource_ != nullptr);
        resource_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(name.size()), name.c_str());
    }

    void destroy()
    {
        SAFE_RELEASE(resource_);
        SAFE_RELEASE(resource_view_);
        SAFE_RELEASE(unordered_access_view_);
    }
};

template<class T>
class ConstBuffer : public Buffer
{
private:
    void internal_initialize(T* data)
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
public:
    ConstBuffer() = default;

    void initialize(T* data)
    {
        assert(resource_ == nullptr);
        static_assert((sizeof(T) & 0x0F) == 0);
        internal_initialize(data);
    }

    // void recreate(T* data) // recreate
    // {
    //     assert(resource_ != nullptr);
    //     resource_->Release();
    //     internal_initialize(data);
    // }
};

template<class T>
class DynamicBuffer : public Buffer
{
public:
    DynamicBuffer() = default;

    void initialize(T* data)
    {
        static_assert((sizeof(T) & 0x0F) == 0);
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
        assert(resource_ != nullptr);
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
private:
    size_t count_ = 0;
public:
    ShaderResource() = default;

    void initialize(const T* data, size_t count)
    {
        count_ = count;
        auto device = Game::inst()->render().device();

        D3D11_BUFFER_DESC buffer_desc;
        buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
        buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        buffer_desc.StructureByteStride = sizeof(T);
        buffer_desc.ByteWidth = sizeof(T) * static_cast<UINT>(count);

        if (data != nullptr) {
            D3D11_SUBRESOURCE_DATA subresource_data;
            subresource_data.pSysMem = data;
            subresource_data.SysMemPitch = 0;
            subresource_data.SysMemSlicePitch = 0;
            D3D11_CHECK(device->CreateBuffer(&buffer_desc, &subresource_data, &resource_));
        } else {
            D3D11_CHECK(device->CreateBuffer(&buffer_desc, nullptr, &resource_));
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc;
        resource_view_desc.BufferEx.FirstElement = 0;
        resource_view_desc.BufferEx.NumElements = static_cast<UINT>(count);
        resource_view_desc.BufferEx.Flags = 0;
        resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
        D3D11_CHECK(device->CreateShaderResourceView(resource_, &resource_view_desc, &resource_view_));
    }

    void update(T* data)
    {
        assert(resource_ != nullptr);
        auto context = Game::inst()->render().context();
        D3D11_MAPPED_SUBRESOURCE mss;
        context->Map(resource_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mss);
        memcpy(mss.pData, data, sizeof(T));
        context->Unmap(resource_, 0);
    }

    size_t size() const { return count_; }
};

// GPU only buffer, no CPU write
template<class T>
class UnorderedAccessBuffer : public Buffer
{
public:
    UnorderedAccessBuffer() = default;

    void initialize(T* data, uint32_t count)
    {
        auto device = Game::inst()->render().device();

        D3D11_BUFFER_DESC buffer_desc;
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        buffer_desc.CPUAccessFlags = 0;
        buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        buffer_desc.StructureByteStride = sizeof(T);
        buffer_desc.ByteWidth = sizeof(T) * count;

        if (data != nullptr) {
            D3D11_SUBRESOURCE_DATA subresource_data;
            subresource_data.pSysMem = data;
            subresource_data.SysMemPitch = 0;
            subresource_data.SysMemSlicePitch = 0;
            D3D11_CHECK(device->CreateBuffer(&buffer_desc, &subresource_data, &resource_));
        } else {
            D3D11_CHECK(device->CreateBuffer(&buffer_desc, nullptr, &resource_));
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc = {};
        resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        resource_view_desc.Buffer.ElementOffset = 0;
        resource_view_desc.Buffer.ElementWidth = count;
        D3D11_CHECK(device->CreateShaderResourceView(resource_, &resource_view_desc, &resource_view_));

        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = count;
        uav_desc.Buffer.Flags = 0;
        D3D11_CHECK(device->CreateUnorderedAccessView(resource_, &uav_desc, &unordered_access_view_));
    }
};

class IndexBuffer : public Buffer
{
private:
    DXGI_FORMAT format_{ DXGI_FORMAT_UNKNOWN };

    void initialize_internal(const void* data, size_t count, size_t sizeof_data);
public:
    IndexBuffer() = default;

    void initialize(const uint32_t* data, size_t count);

    void initialize(const uint16_t* data, size_t count);

    void bind();
};

class VertexBuffer : public Buffer
{
    UINT stride_{ 0U };
public:
    VertexBuffer() = default;

    void initialize(const void* data, size_t count, size_t stride);

    void bind(UINT slot);
};
