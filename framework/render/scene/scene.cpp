#include "core/game.h"
#include "win32/win.h"
#include "render/render.h"
#include "render/camera.h"
#include "render/d3d11_common.h"
#include "render/annotation.h"

#include "scene.h"
#include "model.h"
#include "light.h"
#include "particle_system.h"

Scene::Scene() : uniform_data_{}
{
}

Scene::~Scene()
{
}

void Scene::initialize()
{
    auto device = Game::inst()->render().device();

    {
        D3D11_INPUT_ELEMENT_DESC inputs[] = {
            { "POSITION_UV_X", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL_UV_Y", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        opaque_pass_shader_.set_vs_shader_from_file("./resources/shaders/deferred/opaque_pass.hlsl", "VSMain", nullptr, nullptr);
        opaque_pass_shader_.set_ps_shader_from_file("./resources/shaders/deferred/opaque_pass.hlsl", "PSMain", nullptr, nullptr);
        opaque_pass_shader_.set_input_layout(inputs, std::size(inputs));
#ifndef NDEBUG
        opaque_pass_shader_.set_name("opaque_pass");
#endif
    }

    {
        present_shader_.set_vs_shader_from_file("./resources/shaders/deferred/present_shader.hlsl", "VSMain", nullptr, nullptr);
        present_shader_.set_ps_shader_from_file("./resources/shaders/deferred/present_shader.hlsl", "PSMain", nullptr, nullptr);
#ifndef NDEBUG
        present_shader_.set_name("present");
#endif
    }

    CD3D11_RASTERIZER_DESC opaque_rast_desc = {};
    opaque_rast_desc.CullMode = D3D11_CULL_BACK;
    opaque_rast_desc.FillMode = D3D11_FILL_SOLID;
    opaque_rast_desc.FrontCounterClockwise = true;
    D3D11_CHECK(device->CreateRasterizerState(&opaque_rast_desc, &opaque_rasterizer_state_));

    CD3D11_RASTERIZER_DESC assemble_rast_desc = {};
    assemble_rast_desc.CullMode = D3D11_CULL_NONE;
    assemble_rast_desc.FillMode = D3D11_FILL_SOLID;
    D3D11_CHECK(device->CreateRasterizerState(&assemble_rast_desc, &assemble_rasterizer_state_));

    uniform_buffer_.initialize(sizeof(uniform_data_), D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    D3D11_SAMPLER_DESC tex_sampler_desc{};
    tex_sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    tex_sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    tex_sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    tex_sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    tex_sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    tex_sampler_desc.MinLOD = 0;
    tex_sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    D3D11_CHECK(device->CreateSamplerState(&tex_sampler_desc, &texture_sampler_state_));

    // D3D11_SAMPLER_DESC depth_sampler_desc{};
    // depth_sampler_desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    // depth_sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    // depth_sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    // depth_sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    // depth_sampler_desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    // depth_sampler_desc.MinLOD = 0;
    // depth_sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    // D3D11_CHECK(device->CreateSamplerState(&depth_sampler_desc, &depth_sampler_state_));

    // deferred initialize
    UINT width = UINT(Game::inst()->win().screen_width());
    UINT height = UINT(Game::inst()->win().screen_height());

    D3D11_TEXTURE2D_DESC rtv_tex_desc{};
    rtv_tex_desc.Width = width;
    rtv_tex_desc.Height = height;
    rtv_tex_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    rtv_tex_desc.MipLevels = 1;
    rtv_tex_desc.ArraySize = 1;
    rtv_tex_desc.SampleDesc.Count = 1;
    rtv_tex_desc.SampleDesc.Quality = 0;
    rtv_tex_desc.Usage = D3D11_USAGE_DEFAULT;
    rtv_tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    rtv_tex_desc.CPUAccessFlags = 0;
    rtv_tex_desc.MiscFlags = 0;

    for (uint32_t i = 0; i < gbuffer_count_; ++i) {
        D3D11_CHECK(device->CreateTexture2D(&rtv_tex_desc, nullptr, &deferred_gbuffers_[i]));
        D3D11_CHECK(device->CreateRenderTargetView((ID3D11Resource*)deferred_gbuffers_[i], nullptr, &deferred_gbuffers_target_view_[i]));
        D3D11_CHECK(device->CreateShaderResourceView((ID3D11Resource*)deferred_gbuffers_[i], nullptr, &deferred_gbuffers_view_[i]));
    }
    D3D11_CHECK(device->CreateTexture2D(&rtv_tex_desc, nullptr, &light_buffer_));
    D3D11_CHECK(device->CreateRenderTargetView((ID3D11Resource*)light_buffer_, nullptr, &light_buffer_target_view_));
    D3D11_CHECK(device->CreateShaderResourceView((ID3D11Resource*)light_buffer_, nullptr, &light_buffer_view_));

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
    D3D11_CHECK(device->CreateDepthStencilState(&depth_stencil_desc, &deferred_depth_state_));

    D3D11_TEXTURE2D_DESC depth_desc{};
    depth_desc.Width = width;
    depth_desc.Height = height;
    depth_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    depth_desc.MipLevels = 1;
    depth_desc.ArraySize = 1;
    depth_desc.SampleDesc.Count = 1;
    depth_desc.SampleDesc.Quality = 0;
    depth_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    depth_desc.CPUAccessFlags = 0;
    depth_desc.MiscFlags = 0;
    D3D11_CHECK(device->CreateTexture2D(&depth_desc, nullptr, &deferred_depth_buffer_));

    D3D11_DEPTH_STENCIL_VIEW_DESC ds_desc{};
    ds_desc.Format = DXGI_FORMAT_D32_FLOAT;
    ds_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    ds_desc.Texture2D.MipSlice = 0;
    D3D11_CHECK(device->CreateDepthStencilView(deferred_depth_buffer_, &ds_desc, &deferred_depth_target_view_));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srv_desc.Texture2DArray.MostDetailedMip = 0;
    srv_desc.Texture2DArray.MipLevels = 1;
    srv_desc.Texture2DArray.FirstArraySlice = 0;
    srv_desc.Texture2DArray.ArraySize = 1;
    D3D11_CHECK(device->CreateShaderResourceView(deferred_depth_buffer_, &srv_desc, &deferred_depth_view_));

    // light pass blend state
    D3D11_RENDER_TARGET_BLEND_DESC rt_blend_desc;
    rt_blend_desc.BlendEnable = true;
    rt_blend_desc.SrcBlend = D3D11_BLEND_ONE;
    rt_blend_desc.DestBlend = D3D11_BLEND_ONE;
    rt_blend_desc.BlendOp = D3D11_BLEND_OP_ADD;
    rt_blend_desc.SrcBlendAlpha = D3D11_BLEND_ONE;
    rt_blend_desc.DestBlendAlpha = D3D11_BLEND_ONE;
    rt_blend_desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rt_blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    D3D11_BLEND_DESC blend_desc;
    blend_desc.AlphaToCoverageEnable = false;
    blend_desc.IndependentBlendEnable = false;
    blend_desc.RenderTarget[0] = rt_blend_desc;
    D3D11_CHECK(device->CreateBlendState(&blend_desc, &light_blend_state_));

    D3D11_DEPTH_STENCIL_DESC light_depth_stencil_desc{};
    light_depth_stencil_desc.DepthEnable = true;
    light_depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    light_depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
    device->CreateDepthStencilState(&light_depth_stencil_desc, &light_depth_state_);

    for (auto& l : lights_) {
        l->initialize();
    }
    for (auto& p : particle_systems_) {
        p->initialize();
    }
}

void Scene::destroy()
{
    models_.clear();
    for (auto& l : lights_) {
        l->destroy_resources();
    }
    for (auto& p : particle_systems_) {
        p->destroy_resources();
    }
    lights_.clear();
    uniform_buffer_.destroy();
    SAFE_RELEASE(opaque_rasterizer_state_);
    SAFE_RELEASE(assemble_rasterizer_state_);
    SAFE_RELEASE(texture_sampler_state_);
    // SAFE_RELEASE(depth_sampler_state_);
    SAFE_RELEASE(deferred_depth_state_);
    SAFE_RELEASE(deferred_depth_view_);
    SAFE_RELEASE(deferred_depth_target_view_);
    SAFE_RELEASE(deferred_depth_buffer_);
    for (uint32_t i = 0; i < gbuffer_count_; ++i) {
        SAFE_RELEASE(deferred_gbuffers_target_view_[i]);
        SAFE_RELEASE(deferred_gbuffers_view_[i]);
        SAFE_RELEASE(deferred_gbuffers_[i]);
    }
    opaque_pass_shader_.destroy();
    SAFE_RELEASE(light_depth_state_);
    present_shader_.destroy();
    SAFE_RELEASE(light_blend_state_);
    SAFE_RELEASE(light_buffer_view_);
    SAFE_RELEASE(light_buffer_target_view_);
    SAFE_RELEASE(light_buffer_);
}

void Scene::add_model(Model* model)
{
    models_.push_back(model);
}

void Scene::add_light(Light* light)
{
    lights_.push_back(light);
}

void Scene::add_particle_system(ParticleSystem* particle_system)
{
    particle_systems_.push_back(particle_system);
}

void Scene::update()
{
    auto camera = Game::inst()->render().camera();
    uniform_data_.view_proj = camera->view_proj();
    uniform_data_.inv_view_proj = camera->view_proj().Invert();
    uniform_data_.camera_pos = camera->position();
    uniform_data_.camera_dir = camera->direction();
    uniform_data_.screen_width = Game::inst()->win().screen_width();
    uniform_data_.screen_height = Game::inst()->win().screen_height();

    for (auto& l : lights_) {
        l->update();
    }

    for (auto& p : particle_systems_) {
        p->update();
    }
}

void Scene::draw()
{
    auto context = Game::inst()->render().context();
    auto camera = Game::inst()->render().camera();

    // generate G-Buffers
    {
        Annotation annotation("Generate G-Buffers");
        // restore default render target and depth stencil
        {
            context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
            context->OMSetRenderTargets(gbuffer_count_, deferred_gbuffers_target_view_, deferred_depth_target_view_);

            float clear_color[4] = { 0.f, 0.f, 0.f, 1.f };
            for (uint32_t i = 0; i < gbuffer_count_; ++i) {
                context->ClearRenderTargetView(deferred_gbuffers_target_view_[i], clear_color);
            }

            context->OMSetDepthStencilState(deferred_depth_state_, 0);
            context->ClearDepthStencilView(deferred_depth_target_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0xFF);

            D3D11_VIEWPORT viewport = {};
            viewport.Width = Game::inst()->win().screen_width();
            viewport.Height = Game::inst()->win().screen_height();
            viewport.TopLeftX = 0;
            viewport.TopLeftY = 0;
            viewport.MinDepth = 0;
            viewport.MaxDepth = 1.0f;

            context->RSSetViewports(1, &viewport);
        }

        context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->RSSetState(opaque_rasterizer_state_);

        // draw models
        opaque_pass_shader_.use();
        uniform_buffer_.update_data(&uniform_data_);
        uniform_buffer_.bind(0);

        for (auto& model : models_) {
            model->draw();
        }
    }

    context->OMSetRenderTargets(1, &light_buffer_target_view_, deferred_depth_target_view_);
    float clear_color[4] = { 0.f, 0.f, 0.f, 1.f };
    context->ClearRenderTargetView(light_buffer_target_view_, clear_color);

    // particles
    {
        Annotation annotation("Particles");
        context->OMSetRenderTargets(1, &light_buffer_target_view_, deferred_depth_target_view_);
        for (auto& p : particle_systems_) {
            p->set_depth_shader_resource_view(deferred_depth_view_, deferred_gbuffers_view_[1]);
            p->draw();
        }
    }

    // context->OMSetRenderTargets(1, &light_buffer_target_view_, deferred_depth_target_view_);

    // lights pass
    {
        Annotation annotation("Light pass");
        context->OMSetBlendState(light_blend_state_, nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(light_depth_state_, 0);
        context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->RSSetState(assemble_rasterizer_state_);
        context->PSSetShaderResources(0, gbuffer_count_, deferred_gbuffers_view_);
        // context->PSSetShaderResources(gbuffer_count_, 1, &deferred_depth_view_);
        context->PSSetSamplers(0, 1, &texture_sampler_state_);
        uniform_buffer_.bind(0);

        for (auto& l : lights_) {
            l->draw();
        }
    }

    // present
    {
        Annotation annotation("Present pass");
        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        // restore default render target and depth stencil
        Game::inst()->render().prepare_resources();
        present_shader_.use();
        context->PSSetShaderResources(0, gbuffer_count_, deferred_gbuffers_view_);
        context->PSSetShaderResources(gbuffer_count_, 1, &deferred_depth_view_);
        context->PSSetShaderResources(gbuffer_count_ + 1, 1, &light_buffer_view_);

        context->Draw(3, 0);
    }
}
