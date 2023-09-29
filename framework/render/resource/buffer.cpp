#include "buffer.h"
#include "render/d3d11_common.h"
#include "core/game.h"
#include "render/render.h"

#include <cassert>

Buffer::Buffer()
{
}

Buffer::~Buffer()
{
    assert(resource_ == nullptr);
}

void Buffer::initialize(D3D11_BIND_FLAG bind_flags, void* data, UINT stride, UINT count, D3D11_USAGE usage, D3D11_CPU_ACCESS_FLAG cpu_access, D3D11_RESOURCE_MISC_FLAG misc)
{
    assert(resource_ == nullptr);
    if (bind_flags == D3D11_BIND_CONSTANT_BUFFER) {
        throw std::exception("Use ConstBuffer to create D3D11_BIND_CONSTANT_BUFFER");
    }

    strides_.push_back(stride);
    offsets_.push_back(0); /// TODO: handle offsets

    if (bind_flags == D3D11_BIND_INDEX_BUFFER && stride < 2) { // invalid input
        assert(false);
    }

    buffer_desc_.Usage = usage;
    buffer_desc_.BindFlags = bind_flags;
    buffer_desc_.CPUAccessFlags = cpu_access;
    buffer_desc_.MiscFlags = misc;
    buffer_desc_.StructureByteStride = misc == D3D11_RESOURCE_MISC_BUFFER_STRUCTURED ? stride : 0;
    buffer_desc_.ByteWidth = stride * count;

    subresource_data_.pSysMem = data;
    subresource_data_.SysMemPitch = 0;
    subresource_data_.SysMemSlicePitch = 0;

    auto device = Game::inst()->render().device();
    D3D11_CHECK(device->CreateBuffer(&buffer_desc_, &subresource_data_, &resource_));

    // create shader resource view
    if (bind_flags == D3D11_BIND_SHADER_RESOURCE)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc;
        resource_view_desc.BufferEx.FirstElement = 0;
        resource_view_desc.BufferEx.NumElements = count;
        resource_view_desc.BufferEx.Flags = 0;
        resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
        D3D11_CHECK(device->CreateShaderResourceView(resource_, &resource_view_desc, &resource_view_));
    }
}

void Buffer::set_name(const std::string& name)
{
    assert(resource_ != nullptr);
    resource_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(name.size()), name.c_str());
}

void Buffer::bind(UINT slot)
{
    assert(resource_ != nullptr);
    auto context = Game::inst()->render().context();

    if (buffer_desc_.BindFlags == D3D11_BIND_VERTEX_BUFFER)
    {
        context->IASetVertexBuffers(slot, 1, &resource_, strides_.data(), offsets_.data());
    }
    else if (buffer_desc_.BindFlags == D3D11_BIND_INDEX_BUFFER)
    {
        if (strides_[0] == sizeof(uint16_t)) {
            context->IASetIndexBuffer(resource_, DXGI_FORMAT_R16_UINT, 0);
        } else if (strides_[0] == sizeof(uint32_t)) {
            context->IASetIndexBuffer(resource_, DXGI_FORMAT_R32_UINT, 0);
        }
    }
    else if (buffer_desc_.BindFlags == D3D11_BIND_CONSTANT_BUFFER)
    {
        context->VSSetConstantBuffers(slot, 1, &resource_);
        context->PSSetConstantBuffers(slot, 1, &resource_);
        context->GSSetConstantBuffers(slot, 1, &resource_);
        context->CSSetConstantBuffers(slot, 1, &resource_);
    }
    else if (buffer_desc_.BindFlags == D3D11_BIND_SHADER_RESOURCE)
    {
        context->VSSetShaderResources(slot, 1, &resource_view_);
        context->PSSetShaderResources(slot, 1, &resource_view_);
        context->GSSetShaderResources(slot, 1, &resource_view_);
        context->CSSetShaderResources(slot, 1, &resource_view_);
    }
}

void Buffer::destroy()
{
    SAFE_RELEASE(resource_);
    SAFE_RELEASE(resource_view_);
}

UINT Buffer::count() const
{
    return buffer_desc_.ByteWidth / strides_[0];
}

void ConstBuffer::initialize(UINT size, D3D11_USAGE usage, D3D11_CPU_ACCESS_FLAG cpu_access)
{
    buffer_desc_.Usage = usage;
    buffer_desc_.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc_.CPUAccessFlags = cpu_access;
    buffer_desc_.MiscFlags = 0;
    buffer_desc_.StructureByteStride = 0;
    buffer_desc_.ByteWidth = size;
    assert(size >= 16);

    auto device = Game::inst()->render().device();
    D3D11_CHECK(device->CreateBuffer(&buffer_desc_, nullptr, &resource_));
}

void ConstBuffer::update_data(void* data)
{
    auto context = Game::inst()->render().context();
    D3D11_MAPPED_SUBRESOURCE mss;
    context->Map(resource_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mss);
    memcpy(mss.pData, data, buffer_desc_.ByteWidth);
    context->Unmap(resource_, 0);
}

void StructuredBuffer::initialize(D3D11_BIND_FLAG bind_flags, void* data, UINT stride, UINT count, D3D11_USAGE usage, D3D11_CPU_ACCESS_FLAG cpu_access)
{
    buffer_desc_.Usage = usage;
    buffer_desc_.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    buffer_desc_.CPUAccessFlags = cpu_access;
    buffer_desc_.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc_.StructureByteStride = stride;
    buffer_desc_.ByteWidth = stride * count;

    subresource_data_.pSysMem = data;
    subresource_data_.SysMemPitch = 0;
    subresource_data_.SysMemSlicePitch = 0;

    auto device = Game::inst()->render().device();
    D3D11_CHECK(device->CreateBuffer(&buffer_desc_, &subresource_data_, &resource_));

    D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc;
    resource_view_desc.BufferEx.FirstElement = 0;
    resource_view_desc.BufferEx.NumElements = count;
    resource_view_desc.BufferEx.Flags = 0;
    resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
    resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    D3D11_CHECK(device->CreateShaderResourceView(resource_, &resource_view_desc, &resource_view_));
}

void StructuredBuffer::update_data(void* data)
{
    auto context = Game::inst()->render().context();
    D3D11_MAPPED_SUBRESOURCE mss;
    context->Map(resource_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mss);
    memcpy(mss.pData, data, buffer_desc_.ByteWidth);
    context->Unmap(resource_, 0);
}
