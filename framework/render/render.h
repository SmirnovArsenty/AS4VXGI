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
    ComPtr<ID3D12CommandQueue> compute_queue_;

    // create_swapchain
    ComPtr<IDXGISwapChain3> swapchain_;
    UINT frame_index_;

    // create_rtv_descriptor_heap
    ComPtr<ID3D12DescriptorHeap> rtv_heap_;
    UINT rtv_descriptor_size_{ 0 };

    // create_render_target_views
    ComPtr<ID3D12Resource> render_targets_[swapchain_buffer_count_];

    // create_command_allocator
    ComPtr<ID3D12CommandAllocator> graphics_command_allocator_;
    ComPtr<ID3D12CommandAllocator> compute_command_allocator_;

    // create_descriptor_heap
    ComPtr<ID3D12DescriptorHeap> resource_descriptor_heap_;
    UINT resource_descriptor_size_;
    ComPtr<ID3D12DescriptorHeap> sampler_descriptor_heap_;
    UINT sampler_descriptor_size_;

    // create fence
    HANDLE graphics_fence_event_;
    ComPtr<ID3D12Fence> graphics_fence_;
    UINT64 graphics_fence_value_{};

    HANDLE compute_fence_event_;
    ComPtr<ID3D12Fence> compute_fence_;
    UINT64 compute_fence_value_{};

    // imgui data
    ComPtr<ID3D12DescriptorHeap> imgui_srv_heap_;
    ComPtr<ID3D12GraphicsCommandList> imgui_graphics_command_list_;

    // constant buffers bind slots
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_slots_[15];

    void create_command_queue();
    void create_swapchain();
    void create_rtv_descriptor_heap();
    void create_render_target_views();
    void create_command_allocator();
    void create_descriptor_heap();
    void create_fence();
    void init_imgui();

    void destroy_command_queue();
    void destroy_swapchain();
    void destroy_rtv_descriptor_heap();
    void destroy_render_target_views();
    void destroy_command_allocator();
    void destroy_descriptor_heap();
    void destroy_fence();
    void term_imgui();

    Camera* camera_{ nullptr };
public:
    Render() = default;
    ~Render() = default;

    void initialize();
    void resize();
    void fullscreen(bool);

    void prepare_frame();

    void prepare_imgui();
    void end_imgui();

    void present();

    void destroy_resources();

    Camera* camera() const;

    const ComPtr<ID3D12Device>& device() const;

    const ComPtr<ID3D12CommandQueue>& graphics_queue() const;
    const ComPtr<ID3D12CommandQueue>& compute_queue() const;

    const ComPtr<ID3D12CommandAllocator>& graphics_command_allocator() const;
    const ComPtr<ID3D12CommandAllocator>& compute_command_allocator() const;

    const ComPtr<ID3D12DescriptorHeap>& resource_descriptor_heap() const;
    const ComPtr<ID3D12DescriptorHeap>& sampler_descriptor_heap() const;
};
