#include "buffer.h"

void IndexBuffer::initialize_internal(const void* data, size_t count, size_t sizeof_data)
{
    assert(resource_ == nullptr);
    D3D11_BUFFER_DESC buffer_desc;
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;
    buffer_desc.ByteWidth = static_cast<UINT>(count * sizeof_data);

    D3D11_SUBRESOURCE_DATA subresource_data;
    subresource_data.pSysMem = data;
    subresource_data.SysMemPitch = 0;
    subresource_data.SysMemSlicePitch = 0;

    auto device = Game::inst()->render().device();
    D3D11_CHECK(device->CreateBuffer(&buffer_desc, &subresource_data, &resource_));
}

void IndexBuffer::initialize(const uint32_t* data, size_t count)
{
    format_ = DXGI_FORMAT_R32_UINT;
    initialize_internal(data, count, sizeof(uint32_t));
}

void IndexBuffer::initialize(const uint16_t* data, size_t count)
{
    format_ = DXGI_FORMAT_R16_UINT;
    initialize_internal(data, count, sizeof(uint16_t));
}

void IndexBuffer::bind()
{
    auto context = Game::inst()->render().context();
    context->IASetIndexBuffer(resource_, format_, 0);
}

void VertexBuffer::initialize(const void* data, size_t count, size_t stride)
{
    assert(resource_ == nullptr);
    stride_ = static_cast<UINT>(stride);

    D3D11_BUFFER_DESC buffer_desc;
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;
    buffer_desc.ByteWidth = stride_ * static_cast<UINT>(count);

    D3D11_SUBRESOURCE_DATA subresource_data;
    subresource_data.pSysMem = data;
    subresource_data.SysMemPitch = 0;
    subresource_data.SysMemSlicePitch = 0;

    auto device = Game::inst()->render().device();
    D3D11_CHECK(device->CreateBuffer(&buffer_desc, &subresource_data, &resource_));
}

void VertexBuffer::bind(UINT slot)
{
    auto context = Game::inst()->render().context();
    UINT offsets{ 0 };
    context->IASetVertexBuffers(slot, 1U, &resource_, &stride_, &offsets);
}
