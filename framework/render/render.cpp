#include <chrono>
#include <thread>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>
#include "common.h"
#include "render.h"
#include "core/game.h"
#include "win32/win.h"
#include "win32/input.h"
#include "camera.h"
#include "resource/pipeline.h"

void Render::initialize()
{
    Game* engine = Game::inst();

    HWND hWnd = engine->win().window();
    if (hWnd == NULL)
    {
        OutputDebugString("Window is not initialized!");
        assert(false);
        return;
    }

#if !defined(NDEBUG)
    {
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgi_debug_controller_.GetAddressOf())))) {
            dxgi_debug_controller_->EnableLeakTrackingForThread();
        }
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(d3d_debug_controller_.GetAddressOf())))) {
            d3d_debug_controller_->EnableDebugLayer();
        }
    }
#endif

    HRESULT_CHECK(CreateDXGIFactory(IID_PPV_ARGS(&factory_)));

#if 1
    factory_->EnumWarpAdapter(IID_PPV_ARGS(&adapter_));
    D3D12CreateDevice(adapter_.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_));
#else
    {
        int32_t best_adapter_index = 0;
        int32_t adapter_number = 0;
        size_t best_memory = 0;
        while (true)
        {
            ComPtr<IDXGIAdapter> adapter;
            if (factory_->EnumAdapters(adapter_number, &adapter) != S_OK) {
                // enumerating finnished
                break;
            }
            DXGI_ADAPTER_DESC adapter_desc;
            adapter->GetDesc(&adapter_desc);
            if (best_memory < adapter_desc.DedicatedVideoMemory) {
                best_adapter_index = adapter_number;
                best_memory = adapter_desc.DedicatedVideoMemory;
            }
            ++adapter_number;
        }
        factory_->EnumAdapters(best_adapter_index, &adapter_);
        D3D12CreateDevice(adapter_.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_));
    }
#endif

#if !defined(NDEBUG)

    // enable break on validation errors and warnings
    device_->QueryInterface(IID_PPV_ARGS(info_queue_.GetAddressOf()));

    HRESULT_CHECK(info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true));
    HRESULT_CHECK(info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true));

    D3D12_MESSAGE_SEVERITY muted_severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
    //D3D12_MESSAGE_ID muted_ids[] = {
    //    // D3D12_MESSAGE_ID_...
    //    };
    D3D12_INFO_QUEUE_FILTER filter{
       {},
       {0, nullptr, _countof(muted_severities), muted_severities, 0, nullptr }}; //_countof(muted_ids), muted_ids} };
    info_queue_->PushStorageFilter(&filter);
#endif

    create_command_queue();
    create_swapchain();
    create_rtv_descriptor_heap();
    create_render_target_views();
    create_command_allocator();
    create_descriptor_heap();
    create_fence();
    init_imgui();

    camera_ = new Camera();
    camera_->initialize();
    // camera_->set_camera(Vector3(100.f), Vector3(0.f, 0.f, 1.f));

    // ImGui::CreateContext();
    // ImGuiIO& io = ImGui::GetIO();
    // io.IniFilename = NULL;
    // ImGui_ImplWin32_Init(hWnd);
    // ImGui_ImplDX12_Init(device_.Get(), context_.Get());
}

void Render::create_command_queue()
{
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&graphics_queue_));
    graphics_queue_->SetName(L"Graphics queue");

    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&compute_queue_));
    compute_queue_->SetName(L"Compute queue");
}

void Render::create_swapchain()
{
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
    swapchain_desc.BufferCount = swapchain_buffer_count_;
    swapchain_desc.Width = UINT(Game::inst()->win().screen_width());
    swapchain_desc.Height = UINT(Game::inst()->win().screen_height());
    swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.Flags = 0; // DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = {};
    fullscreen_desc.RefreshRate.Numerator = 60;
    fullscreen_desc.RefreshRate.Denominator = 1;
    fullscreen_desc.Windowed = true;

    ComPtr<IDXGISwapChain1> swapchain;
    factory_->CreateSwapChainForHwnd(graphics_queue_.Get(), Game::inst()->win().window(), &swapchain_desc, &fullscreen_desc, nullptr, &swapchain);
    swapchain.As(&swapchain_);

    // swapchain_->SetMaximumFrameLatency(swapchain_buffer_count_);
    // swapchain_waitable_object_ = swapchain_->GetFrameLatencyWaitableObject();

    factory_->MakeWindowAssociation(Game::inst()->win().window(), DXGI_MWA_NO_ALT_ENTER);

    frame_index_ = swapchain_->GetCurrentBackBufferIndex();
}

void Render::create_rtv_descriptor_heap()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
    rtv_heap_desc.NumDescriptors = swapchain_buffer_count_;
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device_->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(rtv_heap_.ReleaseAndGetAddressOf()));
    rtv_heap_->SetName(L"Main render target heap");

    rtv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void Render::create_render_target_views()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap_->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < swapchain_buffer_count_; ++i) {
        HRESULT_CHECK(swapchain_->GetBuffer(i, IID_PPV_ARGS(render_targets_[i].ReleaseAndGetAddressOf())));
        device_->CreateRenderTargetView(render_targets_[i].Get(), nullptr, rtv_handle);
        rtv_handle.Offset(1, rtv_descriptor_size_);
    }
}

void Render::create_command_allocator()
{
    HRESULT_CHECK(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(graphics_command_allocator_.ReleaseAndGetAddressOf())));
    graphics_command_allocator_->SetName(L"Graphics command allocator");
    graphics_command_allocator_->Reset();
    HRESULT_CHECK(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(compute_command_allocator_.ReleaseAndGetAddressOf())));
    compute_command_allocator_->SetName(L"Compute command allocator");
    compute_command_allocator_->Reset();
}

void Render::create_descriptor_heap()
{
    D3D12_DESCRIPTOR_HEAP_DESC resource_heap_desc = {};
    resource_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    resource_heap_desc.NumDescriptors = 1000;
    resource_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    resource_heap_desc.NodeMask = 0;
    HRESULT_CHECK(device_->CreateDescriptorHeap(&resource_heap_desc, IID_PPV_ARGS(resource_descriptor_heap_.ReleaseAndGetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC sampler_heap_desc = {};
    sampler_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    sampler_heap_desc.NumDescriptors = 2048;
    sampler_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    sampler_heap_desc.NodeMask = 0;
    HRESULT_CHECK(device_->CreateDescriptorHeap(&sampler_heap_desc, IID_PPV_ARGS(sampler_descriptor_heap_.ReleaseAndGetAddressOf())));
}

void Render::create_fence()
{
    HRESULT_CHECK(device_->CreateFence(graphics_fence_value_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(graphics_fence_.ReleaseAndGetAddressOf())));
    graphics_fence_->SetName(L"Graphics fence");
    ++graphics_fence_value_;
    graphics_fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (graphics_fence_event_ == nullptr) {
        assert(!GetLastError());
    }

    HRESULT_CHECK(device_->CreateFence(compute_fence_value_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(compute_fence_.ReleaseAndGetAddressOf())));
    compute_fence_->SetName(L"Compute fence");
    ++compute_fence_value_;
    compute_fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (compute_fence_event_ == nullptr) {
        assert(!GetLastError());
    }
}

void Render::init_imgui()
{
    ImGui::SetCurrentContext(ImGui::CreateContext());

    ImGui_ImplWin32_Init(Game::inst()->win().window());

    D3D12_DESCRIPTOR_HEAP_DESC imgui_srv_heap_desc = {};
    imgui_srv_heap_desc.NumDescriptors = 1;
    imgui_srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    imgui_srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device_->CreateDescriptorHeap(&imgui_srv_heap_desc, IID_PPV_ARGS(imgui_srv_heap_.ReleaseAndGetAddressOf()));

    ImGui_ImplDX12_Init(device_.Get(), swapchain_buffer_count_, DXGI_FORMAT_R8G8B8A8_UNORM, imgui_srv_heap_.Get(),
        imgui_srv_heap_->GetCPUDescriptorHandleForHeapStart(), imgui_srv_heap_->GetGPUDescriptorHandleForHeapStart());

    HRESULT_CHECK(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, graphics_command_allocator_.Get(), nullptr, IID_PPV_ARGS(imgui_graphics_command_list_.ReleaseAndGetAddressOf())));
    imgui_graphics_command_list_->SetName(L"ImGui graphics command list");
    imgui_graphics_command_list_->Close();
}

void Render::destroy_command_queue()
{
    graphics_queue_.Reset();
    compute_queue_.Reset();
}

void Render::destroy_swapchain()
{
    swapchain_.Reset();
}

void Render::destroy_rtv_descriptor_heap()
{
    rtv_heap_.Reset();
}

void Render::destroy_render_target_views()
{
    for (int i = 0; i < swapchain_buffer_count_; ++i) {
        render_targets_[i].Reset();
    }
}

void Render::destroy_command_allocator()
{
    graphics_command_allocator_.Reset();
    compute_command_allocator_.Reset();
}

void Render::destroy_descriptor_heap()
{
    sampler_descriptor_heap_.Reset();
    resource_descriptor_heap_.Reset();
}

void Render::destroy_fence()
{
    CloseHandle(graphics_fence_event_);
    graphics_fence_event_ = nullptr;
    graphics_fence_.Reset();

    CloseHandle(compute_fence_event_);
    compute_fence_event_ = nullptr;
    compute_fence_.Reset();
}

void Render::term_imgui()
{
    imgui_graphics_command_list_.Reset();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Render::resize()
{
    if (!swapchain_) {
        return;
    }

    swapchain_->ResizeBuffers(swapchain_buffer_count_,
                                UINT(Game::inst()->win().screen_width()),
                                UINT(Game::inst()->win().screen_height()),
                                DXGI_FORMAT_R8G8B8A8_UNORM,
                                DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
}

void Render::fullscreen(bool is_fullscreen)
{
    if (!swapchain_) {
        return;
    }

    swapchain_->SetFullscreenState(is_fullscreen, nullptr);

    resize();
}

void Render::prepare_frame()
{
    { // move camera
        float camera_move_delta = Game::inst()->delta_time() * 1e2f;
        const auto& keyboard = Game::inst()->win().input()->keyboard();

        if (keyboard.shift.pressed) {
            camera_move_delta *= 1e1f;
        }
        if (keyboard.ctrl.pressed) {
            camera_move_delta *= 1e-1f;
        }
        camera_->move_forward(camera_move_delta * keyboard.w.pressed);
        camera_->move_right(camera_move_delta * keyboard.d.pressed);
        camera_->move_forward(-camera_move_delta * keyboard.s.pressed);
        camera_->move_right(-camera_move_delta * keyboard.a.pressed);
        camera_->move_up(camera_move_delta * keyboard.space.pressed);
        camera_->move_up(-camera_move_delta * keyboard.c.pressed);
    }
    { // rotate camera
        const float camera_rotate_delta = Game::inst()->delta_time() / 1e1f;
        const auto& mouse = Game::inst()->win().input()->mouse();
        if (mouse.rbutton.pressed)
        { // camera rotation with right button pressed
            if (mouse.delta_y != 0.f) {
                camera_->pitch(-mouse.delta_y * camera_rotate_delta); // negate to convert from Win32 coordinates
            }
            if (mouse.delta_x != 0.f) {
                camera_->yaw(mouse.delta_x * camera_rotate_delta);
            }
        }
    }

    // reset command allocators
    HRESULT_CHECK(graphics_command_allocator()->Reset());
    HRESULT_CHECK(compute_command_allocator()->Reset());

    // update camera GPU resources
    camera_->update();
}

void Render::prepare_imgui()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Render::end_imgui()
{
    // Render dear imgui into screen
    ImGui::Render();

    imgui_graphics_command_list_->Reset(graphics_command_allocator_.Get(), nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = render_targets_[frame_index_].Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    imgui_graphics_command_list_->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap_->GetCPUDescriptorHandleForHeapStart(), frame_index_, rtv_descriptor_size_);
    imgui_graphics_command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);
    imgui_graphics_command_list_->SetDescriptorHeaps(1, imgui_srv_heap_.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), imgui_graphics_command_list_.Get());
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    imgui_graphics_command_list_->ResourceBarrier(1, &barrier);
    imgui_graphics_command_list_->Close();

    graphics_queue()->ExecuteCommandLists(1, (ID3D12CommandList* const*)imgui_graphics_command_list_.GetAddressOf());
}

void Render::present()
{
    HRESULT_CHECK(swapchain_->Present(1, 0));

    // wait execution
    {
        HRESULT_CHECK(graphics_queue_->Signal(graphics_fence_.Get(), graphics_fence_value_));
        HRESULT_CHECK(compute_queue_->Signal(compute_fence_.Get(), compute_fence_value_));

        HRESULT_CHECK(graphics_fence_->SetEventOnCompletion(graphics_fence_value_, graphics_fence_event_));
        HRESULT_CHECK(compute_fence_->SetEventOnCompletion(compute_fence_value_, compute_fence_event_));

        if (graphics_fence_->GetCompletedValue() < graphics_fence_value_) {
            WaitForSingleObject(graphics_fence_event_, INFINITE);
        }
        if (compute_fence_->GetCompletedValue() < compute_fence_value_) {
            WaitForSingleObject(compute_fence_event_, INFINITE);
        }

        ++graphics_fence_value_;
        ++compute_fence_value_;
    }

    frame_index_ = swapchain_->GetCurrentBackBufferIndex();
}

void Render::destroy_resources()
{
    camera_->destroy();
    delete camera_;
    camera_ = nullptr;

    term_imgui();

    destroy_fence();
    destroy_descriptor_heap();
    destroy_command_allocator();
    destroy_render_target_views();
    destroy_rtv_descriptor_heap();
    destroy_swapchain();
    destroy_command_queue();

#if !defined(NDEBUG)
    info_queue_.Reset();
#endif

    device_.Reset();
    factory_.Reset();

#if !defined(NDEBUG)
    d3d_debug_controller_.Reset();

    dxgi_debug_controller_->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
    dxgi_debug_controller_.Reset();
#endif
}

Camera* Render::camera() const
{
    return camera_;
}

const ComPtr<ID3D12Device>& Render::device() const
{
    return device_;
}

const ComPtr<ID3D12CommandQueue>& Render::graphics_queue() const
{
    return graphics_queue_;
}

const ComPtr<ID3D12CommandQueue>& Render::compute_queue() const
{
    return compute_queue_;
}

const ComPtr<ID3D12CommandAllocator>& Render::graphics_command_allocator() const
{
    return graphics_command_allocator_;
}

const ComPtr<ID3D12CommandAllocator>& Render::compute_command_allocator() const
{
    return compute_command_allocator_;
}

const ComPtr<ID3D12DescriptorHeap>& Render::resource_descriptor_heap() const
{
    return resource_descriptor_heap_;
}

const ComPtr<ID3D12DescriptorHeap>& Render::sampler_descriptor_heap() const
{
    return sampler_descriptor_heap_;
}


