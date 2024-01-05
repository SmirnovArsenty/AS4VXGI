#include <chrono>
#include <thread>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx11.h>
#include "render.h"
#include "core/game.h"
#include "win32/win.h"
#include "win32/input.h"
#include "d3d11_common.h"
#include "camera.h"

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

    // create device and swapchain
    D3D_FEATURE_LEVEL featureLevel[] = { D3D_FEATURE_LEVEL_11_1 };

    DXGI_SWAP_CHAIN_DESC swapDesc = {};
    swapDesc.BufferCount = swapchain_buffer_count_;
    swapDesc.BufferDesc.Width = UINT(Game::inst()->win().screen_width());
    swapDesc.BufferDesc.Height = UINT(Game::inst()->win().screen_height());
    swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.OutputWindow = hWnd;
    swapDesc.Windowed = true;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapDesc.SampleDesc.Count = 1;
    swapDesc.SampleDesc.Quality = 0;

    {
        uint32_t create_device_flags = 0;
#ifndef NDEBUG
        create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

#if 1
        // choose best adapter by memory
        Microsoft::WRL::ComPtr<IDXGIFactory> factory;
        D3D11_CHECK(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory));
        int32_t best_adapter_index = 0;
        int32_t adapter_number = 0;
        size_t best_memory = 0;
        while (true)
        {
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
            HRESULT enum_result = factory->EnumAdapters(adapter_number, &adapter);
            if (enum_result != S_OK) {
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
        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        factory->EnumAdapters(best_adapter_index, &adapter);

        ID3D11Device* temp_device;
        ID3D11DeviceContext* temp_context;
        D3D11_CHECK(D3D11CreateDeviceAndSwapChain(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN,
                                                  nullptr, create_device_flags,
                                                  featureLevel, 1, D3D11_SDK_VERSION,
                                                  &swapDesc, &swapchain_,
                                                  &temp_device, nullptr, &temp_context));

        temp_device->QueryInterface<ID3D11Device5>(&device_);
        temp_context->QueryInterface<ID3D11DeviceContext4>(&context_);
        temp_device->Release();
        temp_context->Release();
#else
        ID3D11Device* temp_device;
        ID3D11DeviceContext* temp_context;
        // choose default adapter
        D3D11_CHECK(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
                                                  nullptr, create_device_flags,
                                                  featureLevel, 1, D3D11_SDK_VERSION,
                                                  &swapDesc, &swapchain_,
                                                  &temp_device, nullptr, &temp_context));
        temp_device->QueryInterface<ID3D11Device5>(&device_);
        temp_context->QueryInterface<ID3D11DeviceContext4>(&context_);
        temp_device->Release();
        temp_context->Release();
#endif

#ifndef NDEBUG
        std::string swapchain_name = "default_swapchain";
        swapchain_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(swapchain_name.size()), swapchain_name.c_str());

        std::string context_name = "default_context";
        context_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(context_name.size()), context_name.c_str());
#endif
    }

    create_render_target_view();

    // setup depth stencil
    D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {};

    depth_stencil_desc.DepthEnable = true;
    depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;

    depth_stencil_desc.StencilEnable = false;
    depth_stencil_desc.StencilReadMask = 0;
    depth_stencil_desc.StencilWriteMask = 0;

    depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    D3D11_CHECK(device_->CreateDepthStencilState(&depth_stencil_desc, &depth_stencil_state_));

    create_depth_stencil_texture_and_view();

    camera_ = new Camera();
    camera_->initialize();
    // camera_->set_camera(Vector3(100.f), Vector3(0.f, 0.f, 1.f));

    // initialize debug annotations
    context_->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), reinterpret_cast<void**>(&user_defined_annotation_));

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(device_.Get(), context_.Get());
}

void Render::resize()
{
    if (!swapchain_) {
        return;
    }

    destroy_render_target_view();
    destroy_depth_stencil_texture_and_view();

    {
        swapchain_->ResizeBuffers(swapchain_buffer_count_,
                                  UINT(Game::inst()->win().screen_width()),
                                  UINT(Game::inst()->win().screen_height()),
                                  DXGI_FORMAT_R8G8B8A8_UNORM,
                                  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    }

    create_render_target_view();
    create_depth_stencil_texture_and_view();
}

void Render::fullscreen(bool is_fullscreen)
{
    if (!swapchain_) {
        return;
    }

    // just a resize for now
    // swapchain_->SetFullscreenState(is_fullscreen, nullptr);

    resize();
}

void Render::prepare_frame()
{
    context_->ClearState();

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
    // update camera GPU resources
    camera_->update();
}

void Render::prepare_resources() const
{
    context_->OMSetRenderTargets(1, &render_target_view_, depth_stencil_view_);

    float clear_color[4] = { 0.f, 0.f, 0.f, 1.f };
    context_->ClearRenderTargetView(render_target_view_, clear_color);

    context_->OMSetDepthStencilState(depth_stencil_state_, 0);
    context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0xFF);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = Game::inst()->win().screen_width();
    viewport.Height = Game::inst()->win().screen_height();
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1.0f;

    context_->RSSetViewports(1, &viewport);
}

void Render::prepare_imgui()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Render::end_imgui()
{
    // Render dear imgui into screen
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Render::restore_targets()
{
    context_->OMSetRenderTargets(0, nullptr, nullptr);
}

void Render::end_frame()
{
    D3D11_CHECK(swapchain_->Present(1, 0));
}

void Render::destroy_resources()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext(ImGui::GetCurrentContext());

    user_defined_annotation_->Release();

    camera_->destroy();
    delete camera_;
    camera_ = nullptr;

    SAFE_RELEASE(depth_stencil_state_);

    destroy_depth_stencil_texture_and_view();

    destroy_render_target_view();
}

ID3D11Device5* Render::device() const
{
    return device_.Get();
}

ID3D11DeviceContext4* Render::context() const
{
    return context_.Get();
}

void Render::create_render_target_view()
{
    destroy_render_target_view();
    D3D11_CHECK(swapchain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer_texture_));
#ifndef NDEBUG
    std::string backbuffer_texture_name = "default_backbuffer";
    backbuffer_texture_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(backbuffer_texture_name.size()), backbuffer_texture_name.c_str());
#endif
    D3D11_CHECK(device_->CreateRenderTargetView(backbuffer_texture_, nullptr, &render_target_view_));
#ifndef NDEBUG
    std::string render_target_view_name = "default_render_target_view";
    render_target_view_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(render_target_view_name.size()), render_target_view_name.c_str());
#endif
}

void Render::destroy_render_target_view()
{
    SAFE_RELEASE(backbuffer_texture_);
    SAFE_RELEASE(render_target_view_);
}

void Render::create_depth_stencil_texture_and_view()
{
    destroy_depth_stencil_texture_and_view();

    D3D11_TEXTURE2D_DESC depth_texture_desc;
    backbuffer_texture_->GetDesc(&depth_texture_desc);
    depth_texture_desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    depth_texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    D3D11_CHECK(device_->CreateTexture2D(&depth_texture_desc, nullptr, &depth_stencil_texture_));
#ifndef NDEBUG
    std::string depth_stencil_texture_name = "default_depth_stencil_texture";
    depth_stencil_texture_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(depth_stencil_texture_name.size()), depth_stencil_texture_name.c_str());
#endif

    D3D11_CHECK(device_->CreateDepthStencilView(depth_stencil_texture_, nullptr, &depth_stencil_view_));
#ifndef NDEBUG
    std::string depth_stencil_view_name = "default_depth_stencil_view";
    depth_stencil_view_->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(depth_stencil_view_name.size()), depth_stencil_view_name.c_str());
#endif
}

void Render::destroy_depth_stencil_texture_and_view()
{
    SAFE_RELEASE(depth_stencil_texture_);
    SAFE_RELEASE(depth_stencil_view_);
}

Camera* Render::camera() const
{
    return camera_;
}

ID3DUserDefinedAnnotation* Render::user_defined_annotation() const
{
    return user_defined_annotation_;
}
