#pragma once

#include "core/game.h"
#include "render/common.h"
#include "render/render.h"

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>

using namespace Microsoft::WRL;

template<class T>
class ConstBuffer
{
private:
    static_assert(sizeof(T) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);

    ComPtr<ID3D12Resource> resource_;
    UINT resource_index_;
    D3D12_CPU_DESCRIPTOR_HANDLE resource_view_;
    void* mapped_ptr_ = nullptr;
public:
    ConstBuffer() = default;

    ~ConstBuffer()
    {
        if (mapped_ptr_ != nullptr) {
            CD3DX12_RANGE range(0, 0);
            resource_->Unmap(0, &range);
            mapped_ptr_ = nullptr;
        }
    }

    void initialize()
    {
        auto& device = Game::inst()->render().device();

        HRESULT_CHECK(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(sizeof(T)),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf())));

        resource_index_ = Game::inst()->render().allocate_resource_descriptor(resource_view_);

        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = resource_->GetGPUVirtualAddress();
        desc.SizeInBytes = sizeof(T);
        device->CreateConstantBufferView(&desc, resource_view_);

        CD3DX12_RANGE range(0, 0);
        HRESULT_CHECK(resource_->Map(0, &range, &mapped_ptr_));
    }

    void update(T& data) const
    {
        assert(mapped_ptr_ != nullptr);
        memcpy(mapped_ptr_, &data, sizeof(T));
    }
};

class IndexBuffer
{
private:
    ComPtr<ID3D12Resource> resource_;

    D3D12_INDEX_BUFFER_VIEW view_;

    void* mapped_ptr_ = nullptr;
public:
    IndexBuffer() = default;
    ~IndexBuffer()
    {
        if (mapped_ptr_ != nullptr)
        {
            CD3DX12_RANGE range(0, 0);
            resource_->Unmap(0, &range);
            mapped_ptr_ = nullptr;
        }
    }

    void initialize(const std::vector<UINT32>& indices)
    {
        auto& device = Game::inst()->render().device();

        HRESULT_CHECK(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(UINT32)),
            D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_INDEX_BUFFER,
            nullptr,
            IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf())));

        CD3DX12_RANGE range(0, 0);
        resource_->Map(0, &range, &mapped_ptr_);
        memcpy(mapped_ptr_, indices.data(), indices.size() * sizeof(UINT32));

        view_.BufferLocation = resource_->GetGPUVirtualAddress();
        view_.Format = DXGI_FORMAT_R32_UINT;
        view_.SizeInBytes = UINT(indices.size()) * sizeof(UINT32);
    }

    ID3D12Resource* resource() const
    {
        return resource_.Get();
    }

    D3D12_INDEX_BUFFER_VIEW view() const
    {
        return view_;
    }
};

template<class V>
class VertexBuffer
{
    ComPtr<ID3D12Resource> resource_;

    D3D12_VERTEX_BUFFER_VIEW view_;

    void* mapped_ptr_;
public:
    VertexBuffer() = default;

    ~VertexBuffer()
    {
        if (mapped_ptr_ != nullptr)
        {
            CD3DX12_RANGE range(0, 0);
            resource_->Unmap(0, &range);
            mapped_ptr_ = nullptr;
        }
    }

    void initialize(const std::vector<V>& vertices)
    {
        auto& device = Game::inst()->render().device();

        HRESULT_CHECK(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(V)),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf())));

        CD3DX12_RANGE range(0, 0);
        resource_->Map(0, &range, &mapped_ptr_);
        memcpy(mapped_ptr_, vertices.data(), vertices.size() * sizeof(V));

        view_.BufferLocation = resource_->GetGPUVirtualAddress();
        view_.SizeInBytes = UINT(vertices.size()) * sizeof(V);
        view_.StrideInBytes = sizeof(V);
    }

    ID3D12Resource* resource() const
    {
        return resource_.Get();
    }

    D3D12_VERTEX_BUFFER_VIEW view() const
    {
        return view_;
    }
};
