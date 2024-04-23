#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <string>
#include <vector>
#include <cstdint>
#include <SimpleMath.h>

#if !defined(NDEBUG)
#include <dxgidebug.h>
#endif

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

class GameComponent;
class Camera;

class Render
{
private:
    constexpr static uint32_t swapchain_buffer_count_{ 2 };

#if !defined(NDEBUG)
    // debug
    ComPtr<ID3D12Debug1> d3d_debug_controller_;
    ComPtr<IDXGIDebug1> dxgi_debug_controller_;
    ComPtr<ID3D12InfoQueue> info_queue_;
#endif

    // initialize
    ComPtr<IDXGIFactory4> factory_;
    ComPtr<IDXGIAdapter> adapter_;
    ComPtr<ID3D12Device> device_;

    // create_command_queue
    ComPtr<ID3D12CommandQueue> graphics_queue_;

    // create_swapchain
    ComPtr<IDXGISwapChain3> swapchain_;
    UINT frame_index_;

    UINT rtv_descriptor_size_{ 0 };
    UINT dsv_descriptor_size_{ 0 };
    // create_rtv_descriptor_heap
    // swapchain buffers
    ComPtr<ID3D12DescriptorHeap> rtv_heap_;
    // deferred render targets
    ComPtr<ID3D12DescriptorHeap> dsv_heap_;
    ComPtr<ID3D12DescriptorHeap> gbuffer_heap_;

    // create_render_target_views
    ComPtr<ID3D12Resource> render_targets_[swapchain_buffer_count_];

    // deferred pass render targets
    ComPtr<ID3D12Resource> g_buffers_[swapchain_buffer_count_][8];
    ComPtr<ID3D12Resource> depth_stencil_[swapchain_buffer_count_];

    // create_command_allocator
    ComPtr<ID3D12CommandAllocator> graphics_command_allocator_[swapchain_buffer_count_];

    // create_descriptor_heap
    UINT resource_descriptor_size_;
    ComPtr<ID3D12DescriptorHeap> gpu_resource_descriptor_heap_;
    UINT gpu_resources_allocated_;
    ComPtr<ID3D12DescriptorHeap> cpu_resource_descriptor_heap_;
    UINT cpu_resources_allocated_;

    UINT sampler_descriptor_size_;
    ComPtr<ID3D12DescriptorHeap> sampler_descriptor_heap_;

    // create fence
    HANDLE graphics_fence_event_;
    ComPtr<ID3D12Fence> graphics_fence_;
    UINT64 graphics_fence_value_{};

    // defaults
    CD3DX12_VIEWPORT viewport_;
    CD3DX12_RECT scissor_rect_;

    // internal command list
    ComPtr<ID3D12GraphicsCommandList> cmd_list_[swapchain_buffer_count_];

    // imgui data
    ComPtr<ID3D12DescriptorHeap> imgui_srv_heap_;
    ComPtr<ID3D12GraphicsCommandList> imgui_graphics_command_list_[swapchain_buffer_count_];

    void create_command_queue();
    void create_swapchain();
    void create_rtv_descriptor_heap();
    void create_render_target_views();
    void create_command_allocator();
    void create_descriptor_heap();
    void create_fence();
    void setup_viewport();
    void init_imgui();
    void create_cmd_list();

    void destroy_command_queue();
    void destroy_swapchain();
    void destroy_rtv_descriptor_heap();
    void destroy_render_target_views();
    void destroy_command_allocator();
    void destroy_descriptor_heap();
    void destroy_fence();
    void term_imgui();
    void destroy_cmd_list();

    Camera* camera_{ nullptr };
public:
    Render() = default;
    ~Render() = default;

    void initialize();
    void resize();
    void fullscreen(bool);

    void prepare_frame();
    void end_frame();

    void prepare_imgui();
    void end_imgui();

    void present();

    void destroy_resources();

    Camera* camera() const;

    ComPtr<ID3D12Device> device() const;

    ComPtr<ID3D12CommandQueue> graphics_queue() const;

    ComPtr<ID3D12CommandAllocator> graphics_command_allocator() const;

    ComPtr<ID3D12DescriptorHeap> resource_descriptor_heap() const;
    ComPtr<ID3D12DescriptorHeap> sampler_descriptor_heap() const;

    D3D12_CPU_DESCRIPTOR_HANDLE render_target() const;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> gbuffer_render_targets() const;
    D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil() const;

    const D3D12_VIEWPORT& viewport() const;
    const D3D12_RECT& scissor_rect() const;

    UINT allocate_gpu_resource_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle);
    UINT allocate_cpu_resource_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle);
};
