#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <string>
#include <vector>
#include <cstdint>
#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

class GameComponent;
class Camera;
class ShaderCache;

class Render
{
private:
    // main D3D11 units
    Microsoft::WRL::ComPtr<ID3D11Device> device_{ nullptr };
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_{ nullptr };
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain_{ nullptr };

    constexpr static uint32_t swapchain_buffer_count_{ 2 };

    // backbuffer[0] texture and render target
    ID3D11Texture2D* backbuffer_texture_{ nullptr };
    ID3D11RenderTargetView* render_target_view_{ nullptr };

    // depth stencil
    ID3D11Texture2D* depth_stencil_texture_{ nullptr };
    ID3D11DepthStencilView* depth_stencil_view_{ nullptr };
    ID3D11DepthStencilState* depth_stencil_state_{ nullptr };

    void create_render_target_view();
    void destroy_render_target_view();
    void create_depth_stencil_texture_and_view();
    void destroy_depth_stencil_texture_and_view();

    Camera* camera_{ nullptr };

    ShaderCache* shader_cache_{ nullptr };

    ID3DUserDefinedAnnotation* user_defined_annotation_{ nullptr };

public:
    Render() = default;
    ~Render() = default;

    void initialize();
    void resize();
    void fullscreen(bool);

    void prepare_frame();
    void prepare_resources() const;

    void prepare_imgui();
    void end_imgui();

    void restore_targets();
    void end_frame();

    void destroy_resources();

    ID3D11Device* device() const;
    ID3D11DeviceContext* context() const;

    Camera* camera() const;

    ShaderCache* shader_cache() const;

    ID3DUserDefinedAnnotation* user_defined_annotation() const;
};
