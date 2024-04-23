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
    D3D12_GPU_DESCRIPTOR_HANDLE resource_view_gpu_;
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

        SAFE_RELEASE(resource_);
    }

    void initialize()
    {
        auto device = Game::inst()->render().device();

        HRESULT_CHECK(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(sizeof(T)),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf())));

        resource_index_ = Game::inst()->render().allocate_gpu_resource_descriptor(resource_view_, resource_view_gpu_);

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

    const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_descriptor_handle() const
    {
        return resource_view_;
    }

    const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_descriptor_handle() const
    {
        return resource_view_gpu_;
    }
};

template<class T>
class ShaderResource
{
private:
    ID3D12Resource* resource_;
    UINT resource_index_;
    D3D12_CPU_DESCRIPTOR_HANDLE resource_view_;
    D3D12_GPU_DESCRIPTOR_HANDLE resource_view_gpu_;

    UINT size_;
public:
    ShaderResource() = default;
    ~ShaderResource()
    {
        if (resource_) {
            resource_->Release();
            resource_ = nullptr;
        }
    }

    void initialize(T* data, UINT size)
    {
        size_ = size;

        auto device = Game::inst()->render().device();

        HRESULT_CHECK(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(size * sizeof(T)),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&resource_)));

        { // copy resource on gpu
            ID3D12Resource* tmp;
            ID3D12GraphicsCommandList* copy_cmd;
            {
                HRESULT_CHECK(device->CreateCommittedResource(
                    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                    D3D12_HEAP_FLAG_NONE,
                    &CD3DX12_RESOURCE_DESC::Buffer(size * sizeof(T)),
                    D3D12_RESOURCE_STATE_COPY_SOURCE,
                    nullptr,
                    IID_PPV_ARGS(&tmp)));
                tmp->SetName(L"Temporary object to upload data");

                CD3DX12_RANGE range(0, 0);
                void* mapped_ptr = nullptr;
                HRESULT_CHECK(tmp->Map(0, &range, &mapped_ptr));
                memcpy(mapped_ptr, data, sizeof(T) * size);
                tmp->Unmap(0, &range);
            }
            {
                device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, Game::inst()->render().graphics_command_allocator().Get(), nullptr, IID_PPV_ARGS(&copy_cmd));
                PIXBeginEvent(copy_cmd, PIX_COLOR(0xFF, 0xFF, 0x00), "Copy shader resource");
                copy_cmd->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource_, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
                copy_cmd->CopyResource(resource_, tmp);
                copy_cmd->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource_, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
                PIXEndEvent(copy_cmd);
                HRESULT_CHECK(copy_cmd->Close());

                Game::inst()->render().graphics_queue()->ExecuteCommandLists(1, (ID3D12CommandList**)&copy_cmd);
            }

            { // sync
                ID3D12Fence* fence;
                device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
                HANDLE ev = CreateEvent(nullptr, false, false, nullptr);
                fence->SetEventOnCompletion(1, ev);
                Game::inst()->render().graphics_queue()->Signal(fence, 1);
                if (fence->GetCompletedValue() < 1)
                {
                    WaitForSingleObject(ev, INFINITE);
                }
                CloseHandle(ev);
                fence->Release();
            }

            tmp->Release();
            copy_cmd->Release();
        }

        resource_index_ = Game::inst()->render().allocate_gpu_resource_descriptor(resource_view_, resource_view_gpu_);

        D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
        desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.Buffer.FirstElement = 0;
        desc.Buffer.NumElements = size;
        desc.Buffer.StructureByteStride = sizeof(T);
        desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        device->CreateShaderResourceView(resource_, &desc, resource_view_);
    }

    UINT size() const
    {
        return size_;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_descriptor_handle() const
    {
        return resource_view_;
    }

    const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_descriptor_handle() const
    {
        return resource_view_gpu_;
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

        SAFE_RELEASE(resource_);
    }

    void initialize(const std::vector<UINT32>& indices)
    {
        auto device = Game::inst()->render().device();

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

    const D3D12_INDEX_BUFFER_VIEW& view() const
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

        SAFE_RELEASE(resource_);
    }

    void initialize(const std::vector<V>& vertices)
    {
        auto device = Game::inst()->render().device();

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

    const D3D12_VERTEX_BUFFER_VIEW& view() const
    {
        return view_;
    }
};
