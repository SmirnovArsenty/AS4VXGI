#pragma once

#include <vector>
#include <string>

#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

#include "render/resource/buffer.h"
#include "render/resource/shader.h"

class Scene
{
public:
    Scene();
    ~Scene();

    void initialize();
    void destroy();

    // TODO: load scene from xml description

    void add_model(class Model* model);

    void add_light(class Light* light);

    void add_particle_system(class ParticleSystem* particle_system);

    void update();
    void draw();
private:
    std::vector<class Model*> models_;
    std::vector<class Light*> lights_;
    std::vector<class ParticleSystem*> particle_systems_;

    ConstBuffer uniform_buffer_;
    struct
    {
        Matrix view_proj;
        Matrix inv_view_proj;
        Vector3 camera_pos;
        float screen_width;
        Vector3 camera_dir;
        float screen_height;
    } uniform_data_;

    // opaque pass
    GraphicsShader opaque_pass_shader_;
    ID3D11RasterizerState* opaque_rasterizer_state_{ nullptr };

    ID3D11DepthStencilState* light_depth_state_{ nullptr };

    // assemble
    GraphicsShader present_shader_;
    ID3D11SamplerState* texture_sampler_state_{ nullptr };
    ID3D11RasterizerState* assemble_rasterizer_state_{ nullptr };

    constexpr static uint32_t gbuffer_count_ = 5;
    // position
    // normal
    // diffuse
    // specular
    // ambient
    ID3D11Texture2D* deferred_gbuffers_[gbuffer_count_]{ nullptr };
    ID3D11ShaderResourceView* deferred_gbuffers_view_[gbuffer_count_]{ nullptr };
    ID3D11RenderTargetView* deferred_gbuffers_target_view_[gbuffer_count_]{ nullptr };
    // depth
    ID3D11DepthStencilState* deferred_depth_state_{ nullptr };
    ID3D11Texture2D* deferred_depth_buffer_{ nullptr };
    ID3D11ShaderResourceView* deferred_depth_view_{ nullptr };
    ID3D11DepthStencilView* deferred_depth_target_view_{ nullptr };

    // light pass buffer
    ID3D11BlendState* light_blend_state_{ nullptr };
    ID3D11Texture2D* light_buffer_{ nullptr };
    ID3D11ShaderResourceView* light_buffer_view_{ nullptr };
    ID3D11RenderTargetView* light_buffer_target_view_{ nullptr };
};
