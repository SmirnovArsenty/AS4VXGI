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

#if !defined(NDEBUG)

    // enable break on validation errors and warnings
    device_->QueryInterface(IID_PPV_ARGS(info_queue_.GetAddressOf()));

    HRESULT_CHECK(info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true));
    HRESULT_CHECK(info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false));

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

    setup_viewport();

    init_imgui();

    create_cmd_list();

    camera_ = new Camera();
    camera_->initialize();
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

    factory_->MakeWindowAssociation(Game::inst()->win().window(), DXGI_MWA_NO_ALT_ENTER);

    frame_index_ = swapchain_->GetCurrentBackBufferIndex();
}

void Render::create_rtv_descriptor_heap()
{
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = swapchain_buffer_count_;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HRESULT_CHECK(device_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(rtv_heap_.ReleaseAndGetAddressOf())));
    rtv_heap_->SetName(L"Main render target heap (swapchain render targets)");
    rtv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    heap_desc.NumDescriptors = 8 * swapchain_buffer_count_;
    HRESULT_CHECK(device_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(gbuffer_heap_.ReleaseAndGetAddressOf())));
    gbuffer_heap_->SetName(L"Deferred Rendering Render Targets Heap (GBuffers)");

    heap_desc.NumDescriptors = swapchain_buffer_count_;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    HRESULT_CHECK(device_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(dsv_heap_.ReleaseAndGetAddressOf())));
    dsv_heap_->SetName(L"Deferred Rendering Depth Stencil Buffer");
    dsv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void Render::create_render_target_views()
{
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap_->GetCPUDescriptorHandleForHeapStart());

        for (int i = 0; i < swapchain_buffer_count_; ++i) {
            HRESULT_CHECK(swapchain_->GetBuffer(i, IID_PPV_ARGS(render_targets_[i].ReleaseAndGetAddressOf())));
            device_->CreateRenderTargetView(render_targets_[i].Get(), nullptr, rtv_handle);
            rtv_handle.Offset(1, rtv_descriptor_size_);
        }
    }

    {
        for (int i = 0; i < swapchain_buffer_count_; ++i) {
            D3D12_RESOURCE_DESC rt_desc = render_targets_[i]->GetDesc();

            rt_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            for (int j = 0; j < _countof(g_buffers_[i]); ++j) {
                CD3DX12_CPU_DESCRIPTOR_HANDLE gbuffer_handle(gbuffer_heap_->GetCPUDescriptorHandleForHeapStart(), i * _countof(g_buffers_[i]) + j, rtv_descriptor_size_);
                D3D12_CLEAR_VALUE clear{};
                clear.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                HRESULT_CHECK(device_->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                    D3D12_HEAP_FLAG_NONE,
                    &rt_desc,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    &clear,
                    IID_PPV_ARGS(g_buffers_[i][j].ReleaseAndGetAddressOf())));
                device_->CreateRenderTargetView(g_buffers_[i][j].Get(), nullptr, gbuffer_handle);
            }

            {
                CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(dsv_heap_->GetCPUDescriptorHandleForHeapStart(), i, dsv_descriptor_size_);
                rt_desc.Format = DXGI_FORMAT_D32_FLOAT;
                rt_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
                D3D12_CLEAR_VALUE clear{};
                clear.Format = DXGI_FORMAT_D32_FLOAT;
                clear.DepthStencil.Depth = 1.f;
                clear.DepthStencil.Stencil = 0;
                HRESULT_CHECK(device_->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                    D3D12_HEAP_FLAG_NONE,
                    &rt_desc,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ,
                    &clear,
                    IID_PPV_ARGS(depth_stencil_[i].ReleaseAndGetAddressOf())));
                device_->CreateDepthStencilView(depth_stencil_[i].Get(), nullptr, dsv_handle);
            }
        }
    }
}

void Render::create_command_allocator()
{
    for (int i = 0; i < swapchain_buffer_count_; ++i) {
        HRESULT_CHECK(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(graphics_command_allocator_[i].ReleaseAndGetAddressOf())));
        graphics_command_allocator_[i]->SetName(L"Graphics command allocator");
        graphics_command_allocator_[i]->Reset();
        HRESULT_CHECK(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(compute_command_allocator_[i].ReleaseAndGetAddressOf())));
        compute_command_allocator_[i]->SetName(L"Compute command allocator");
        compute_command_allocator_[i]->Reset();
    }
}

void Render::create_descriptor_heap()
{
    D3D12_DESCRIPTOR_HEAP_DESC resource_heap_desc = {};
    resource_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    resource_heap_desc.NumDescriptors = 1000;
    resource_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    resource_heap_desc.NodeMask = 0;
    HRESULT_CHECK(device_->CreateDescriptorHeap(&resource_heap_desc, IID_PPV_ARGS(resource_descriptor_heap_.ReleaseAndGetAddressOf())));
    resource_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    resources_allocated_ = 0;

    D3D12_DESCRIPTOR_HEAP_DESC sampler_heap_desc = {};
    sampler_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    sampler_heap_desc.NumDescriptors = 2048;
    sampler_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    sampler_heap_desc.NodeMask = 0;
    HRESULT_CHECK(device_->CreateDescriptorHeap(&sampler_heap_desc, IID_PPV_ARGS(sampler_descriptor_heap_.ReleaseAndGetAddressOf())));
    sampler_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
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

void Render::setup_viewport()
{
    float width = Game::inst()->win().screen_width();
    float height = Game::inst()->win().screen_height();
    viewport_ = CD3DX12_VIEWPORT(0.0f, 0.0f, width, height);
    scissor_rect_ = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
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

    for (int i = 0; i < swapchain_buffer_count_; ++i) {
        HRESULT_CHECK(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, graphics_command_allocator_[i].Get(), nullptr, IID_PPV_ARGS(imgui_graphics_command_list_[i].ReleaseAndGetAddressOf())));
        imgui_graphics_command_list_[i]->SetName(L"ImGui graphics command list");
        imgui_graphics_command_list_[i]->Close();
    }
}

void Render::create_cmd_list()
{
    for (int i = 0; i < swapchain_buffer_count_; ++i) {
        HRESULT_CHECK(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, graphics_command_allocator_[i].Get(), nullptr, IID_PPV_ARGS(cmd_list_[i].ReleaseAndGetAddressOf())));
        cmd_list_[i]->SetName(L"Render internal cmd list");
        cmd_list_[i]->Close();
    }
}

void Render::destroy_command_queue()
{
    SAFE_RELEASE(graphics_queue_);
    SAFE_RELEASE(compute_queue_);
}

void Render::destroy_swapchain()
{
    SAFE_RELEASE(swapchain_);
}

void Render::destroy_rtv_descriptor_heap()
{
    SAFE_RELEASE(dsv_heap_);
    SAFE_RELEASE(gbuffer_heap_);
    SAFE_RELEASE(rtv_heap_);
}

void Render::destroy_render_target_views()
{
    for (int i = 0; i < swapchain_buffer_count_; ++i) {
        SAFE_RELEASE(render_targets_[i]);
    }
}

void Render::destroy_command_allocator()
{
    for (int i = 0; i < swapchain_buffer_count_; ++i) {
        SAFE_RELEASE(graphics_command_allocator_[i]);
        SAFE_RELEASE(compute_command_allocator_[i]);
    }
}

void Render::destroy_descriptor_heap()
{
    SAFE_RELEASE(sampler_descriptor_heap_);
    SAFE_RELEASE(resource_descriptor_heap_);
}

void Render::destroy_fence()
{
    CloseHandle(graphics_fence_event_);
    graphics_fence_event_ = nullptr;
    SAFE_RELEASE(graphics_fence_);

    CloseHandle(compute_fence_event_);
    compute_fence_event_ = nullptr;
    SAFE_RELEASE(compute_fence_);
}

void Render::term_imgui()
{
    for (int i = 0; i < swapchain_buffer_count_; ++i) {
        SAFE_RELEASE(imgui_graphics_command_list_[i]);
    }
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Render::destroy_cmd_list()
{
    for (int i = 0; i < swapchain_buffer_count_; ++i) {
        SAFE_RELEASE(cmd_list_[i]);
    }
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

    setup_viewport();
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

    HRESULT_CHECK(cmd_list_[frame_index_]->Reset(graphics_command_allocator().Get(), nullptr));
    PIXBeginEvent(cmd_list_[frame_index_].Get(), 0xFFFFFFFF, "Prepare frame");

    cmd_list_[frame_index_]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(render_targets_[frame_index_].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    cmd_list_[frame_index_]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(depth_stencil_[frame_index_].Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

    FLOAT clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
    cmd_list_[frame_index_]->ClearRenderTargetView(render_target(), clear_color, 0, nullptr);
    cmd_list_[frame_index_]->ClearDepthStencilView(depth_stencil(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

    PIXEndEvent(cmd_list_[frame_index_].Get());
    HRESULT_CHECK(cmd_list_[frame_index_]->Close());
    graphics_queue()->ExecuteCommandLists(1, (ID3D12CommandList*const*)cmd_list_[frame_index_].GetAddressOf());

    // update camera GPU resources
    camera_->update();
}

void Render::end_frame()
{
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

    imgui_graphics_command_list_[frame_index_]->Reset(graphics_command_allocator().Get(), nullptr);
    PIXBeginEvent(imgui_graphics_command_list_[frame_index_].Get(), 0xFFFFFFFF, "ImGui");

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap_->GetCPUDescriptorHandleForHeapStart(), frame_index_, rtv_descriptor_size_);
    imgui_graphics_command_list_[frame_index_]->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);
    imgui_graphics_command_list_[frame_index_]->SetDescriptorHeaps(1, imgui_srv_heap_.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), imgui_graphics_command_list_[frame_index_].Get());

    PIXEndEvent(imgui_graphics_command_list_[frame_index_].Get());
    imgui_graphics_command_list_[frame_index_]->Close();

    graphics_queue()->ExecuteCommandLists(1, (ID3D12CommandList* const*)imgui_graphics_command_list_[frame_index_].GetAddressOf());
}

void Render::present()
{
    HRESULT_CHECK(cmd_list_[frame_index_]->Reset(graphics_command_allocator().Get(), nullptr));
    PIXBeginEvent(cmd_list_[frame_index_].Get(), 0xFFFFFFFF, "render target transition to present");
    cmd_list_[frame_index_]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(render_targets_[frame_index_].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
    cmd_list_[frame_index_]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(depth_stencil_[frame_index_].Get(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ));
    PIXEndEvent(cmd_list_[frame_index_].Get());
    HRESULT_CHECK(cmd_list_[frame_index_]->Close());
    graphics_queue()->ExecuteCommandLists(1, (ID3D12CommandList* const*)cmd_list_[frame_index_].GetAddressOf());

    HRESULT_CHECK(swapchain_->Present(1, 0));

    // wait execution
    {
        PIXBeginEvent(graphics_queue_.Get(), 0xFFFFFFFF, "sync after present");
        HRESULT_CHECK(graphics_queue_->Signal(graphics_fence_.Get(), graphics_fence_value_));
        // HRESULT_CHECK(compute_queue_->Signal(compute_fence_.Get(), compute_fence_value_));

        HRESULT_CHECK(graphics_fence_->SetEventOnCompletion(graphics_fence_value_, graphics_fence_event_));
        // HRESULT_CHECK(compute_fence_->SetEventOnCompletion(compute_fence_value_, compute_fence_event_));

        if (graphics_fence_->GetCompletedValue() < graphics_fence_value_) {
            WaitForSingleObject(graphics_fence_event_, INFINITE);
        }
        // if (compute_fence_->GetCompletedValue() < compute_fence_value_) {
        //     WaitForSingleObject(compute_fence_event_, INFINITE);
        // }

        ++graphics_fence_value_;
        // ++compute_fence_value_;

        PIXEndEvent(graphics_queue_.Get());
    }

    frame_index_ = swapchain_->GetCurrentBackBufferIndex();
}

void Render::destroy_resources()
{
    camera_->destroy();
    delete camera_;
    camera_ = nullptr;

    destroy_cmd_list();

    term_imgui();

    destroy_fence();
    destroy_descriptor_heap();
    destroy_command_allocator();
    destroy_render_target_views();
    destroy_rtv_descriptor_heap();
    destroy_swapchain();
    destroy_command_queue();

#if !defined(NDEBUG)
    SAFE_RELEASE(info_queue_);
#endif

    SAFE_RELEASE(device_);
    SAFE_RELEASE(factory_);

#if !defined(NDEBUG)
    SAFE_RELEASE(d3d_debug_controller_);

    dxgi_debug_controller_->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
    SAFE_RELEASE(dxgi_debug_controller_);
#endif
}

Camera* Render::camera() const
{
    return camera_;
}

ComPtr<ID3D12Device> Render::device() const
{
    return device_;
}

ComPtr<ID3D12CommandQueue> Render::graphics_queue() const
{
    return graphics_queue_;
}

ComPtr<ID3D12CommandQueue> Render::compute_queue() const
{
    return compute_queue_;
}

ComPtr<ID3D12CommandAllocator> Render::graphics_command_allocator() const
{
    return graphics_command_allocator_[frame_index_];
}

ComPtr<ID3D12CommandAllocator> Render::compute_command_allocator() const
{
    return compute_command_allocator_[frame_index_];
}

ComPtr<ID3D12DescriptorHeap> Render::resource_descriptor_heap() const
{
    return resource_descriptor_heap_;
}

ComPtr<ID3D12DescriptorHeap> Render::sampler_descriptor_heap() const
{
    return sampler_descriptor_heap_;
}

D3D12_CPU_DESCRIPTOR_HANDLE Render::render_target() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(rtv_heap_->GetCPUDescriptorHandleForHeapStart(), frame_index_, rtv_descriptor_size_);
}

D3D12_CPU_DESCRIPTOR_HANDLE Render::depth_stencil() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(dsv_heap_->GetCPUDescriptorHandleForHeapStart(), frame_index_, dsv_descriptor_size_);
}

const D3D12_VIEWPORT& Render::viewport() const
{
    return viewport_;
}

const D3D12_RECT& Render::scissor_rect() const
{
    return scissor_rect_;
}

UINT Render::allocate_resource_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle)
{
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(resource_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(resources_allocated_, resource_descriptor_size_);
        cpu_handle = handle;
    }
    {
        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(resource_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(resources_allocated_, resource_descriptor_size_);
        gpu_handle = handle;
    }
    return resources_allocated_++;
}
