#include <imgui/imgui.h>

#include "win32/win.h"
#include "render/render.h"
#include "render/camera.h"
#include "render/d3d11_common.h"
#include "render/annotation.h"

#include "particle_system.h"

ParticleSystem::ParticleSystem(uint32_t max_particles, Vector3 origin)
    : max_particles_count_{ max_particles }
{
    uniform_data_.global_time = 0;
    emitter_data_.origin = origin;
}

ParticleSystem::~ParticleSystem()
{

}

void ParticleSystem::initialize()
{
    auto device = Game::inst()->render().device();

    uniform_buffer_.initialize(sizeof(uniform_data_));

    D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    buffer_desc.StructureByteStride = sizeof(Particle);
    buffer_desc.ByteWidth = sizeof(Particle) * max_particles_count_;
    D3D11_CHECK(device->CreateBuffer(&buffer_desc, nullptr, &particle_pool_));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.ElementOffset = 0;
    srv_desc.Buffer.ElementWidth = max_particles_count_;
    D3D11_CHECK(device->CreateShaderResourceView(particle_pool_, &srv_desc, &particle_pool_SRV_));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    uav_desc.Format = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = max_particles_count_;
    uav_desc.Buffer.Flags = 0;
    D3D11_CHECK(device->CreateUnorderedAccessView(particle_pool_, &uav_desc, &particle_pool_UAV_));

    // buffer_desc.ByteWidth = 16 * max_particles_count_;
    // buffer_desc.StructureByteStride = 16;
    // D3D11_CHECK(device->CreateBuffer(&buffer_desc, 0, &view_space_particle_positions_));
    // D3D11_CHECK(device->CreateShaderResourceView(view_space_particle_positions_, &srv_desc, &view_space_particle_positions_SRV_));
    // D3D11_CHECK(device->CreateUnorderedAccessView(view_space_particle_positions_, &uav_desc, &view_space_particle_positions_UAV_));

    buffer_desc.StructureByteStride = sizeof(uint32_t);
    buffer_desc.ByteWidth = sizeof(uint32_t) * max_particles_count_;
    D3D11_CHECK(device->CreateBuffer(&buffer_desc, nullptr, &dead_list_));

    uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
    D3D11_CHECK(device->CreateUnorderedAccessView(dead_list_, &uav_desc, &dead_list_UAV_));

    buffer_desc.StructureByteStride = sizeof(float) * 2;
    buffer_desc.ByteWidth = (sizeof(float) * 2) * max_particles_count_;
    device->CreateBuffer(&buffer_desc, nullptr, &sort_list_);

    D3D11_CHECK(device->CreateShaderResourceView(sort_list_, &srv_desc, &sort_list_SRV_));

    uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
    D3D11_CHECK(device->CreateUnorderedAccessView(sort_list_, &uav_desc, &sort_list_UAV_));

    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    buffer_desc.ByteWidth = 5 * sizeof(uint32_t);
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    D3D11_CHECK(device->CreateBuffer(&buffer_desc, nullptr, &indirect_args_));

    ZeroMemory(&uav_desc, sizeof(uav_desc));
    uav_desc.Format = DXGI_FORMAT_R32_UINT;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = 5;
    uav_desc.Buffer.Flags = 0;
    D3D11_CHECK(device->CreateUnorderedAccessView(indirect_args_, &uav_desc, &indirect_args_UAV_));

    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.ByteWidth = max_particles_count_ * 6 * sizeof(UINT);
    buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
    buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA data;

    UINT* indices = new UINT[max_particles_count_ * 6];
    data.pSysMem = indices;
    data.SysMemPitch = 0;
    data.SysMemSlicePitch = 0;

    UINT base = 0;
    for (uint32_t i = 0; i < max_particles_count_; i++)
    {
        indices[0] = base + 0;
        indices[1] = base + 1;
        indices[2] = base + 2;

        indices[3] = base + 2;
        indices[4] = base + 1;
        indices[5] = base + 3;

        base += 4;
        indices += 6;
    }
    D3D11_CHECK(device->CreateBuffer(&buffer_desc, &data, &index_buffer_));
    delete[] data.pSysMem;

#ifndef NDEBUG
    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage = D3D11_USAGE_STAGING;
    buffer_desc.BindFlags = 0;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    buffer_desc.ByteWidth = sizeof(UINT);
    D3D11_CHECK(device->CreateBuffer(&buffer_desc, nullptr, &debug_counter_buffer_));
#endif

    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.ByteWidth = 4 * sizeof(UINT);
    D3D11_CHECK(device->CreateBuffer(&buffer_desc, nullptr, &dead_list_const_buffer_));
    D3D11_CHECK(device->CreateBuffer(&buffer_desc, nullptr, &sort_list_const_buffer_));

    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.ByteWidth = sizeof(EmitterParams);
    D3D11_CHECK(device->CreateBuffer(&buffer_desc, nullptr, &emitter_const_buffer_));

    D3D11_BLEND_DESC blend_desc;
    ZeroMemory(&blend_desc, sizeof(D3D11_BLEND_DESC));
    blend_desc.AlphaToCoverageEnable = false;
    blend_desc.IndependentBlendEnable = false;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTarget[0].BlendEnable = true;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;//SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;//INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    D3D11_CHECK(device->CreateBlendState(&blend_desc, &blend_state_));

    CD3D11_DEPTH_STENCIL_DESC depth_desc{};
    depth_desc.DepthEnable = true;
    depth_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depth_desc.DepthFunc = D3D11_COMPARISON_LESS;
    device->CreateDepthStencilState(&depth_desc, &depth_state_on_);
    depth_desc.DepthEnable = false;
    device->CreateDepthStencilState(&depth_desc, &depth_state_off_);

    CD3D11_RASTERIZER_DESC rasterizer_desc{};
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_desc.CullMode = D3D11_CULL_NONE;
    D3D11_CHECK(device->CreateRasterizerState(&rasterizer_desc, &rasterizer_state_));

    init_dead_list_.set_compute_shader_from_file("./resources/shaders/particles/init_dead_list.hlsl", "CSMain");
    emit_.set_compute_shader_from_file("./resources/shaders/particles/emit.hlsl", "CSMain");
    simulate_.set_compute_shader_from_file("./resources/shaders/particles/simulate.hlsl", "CSMain");
    reset_.set_compute_shader_from_file("./resources/shaders/particles/reset.hlsl", "CSMain");

    render_.set_vs_shader_from_file("./resources/shaders/particles/render.hlsl", "VSMain");
    render_.set_ps_shader_from_file("./resources/shaders/particles/render.hlsl", "PSMain");

    sort_lib_.init();

    reset_flag_ = true;

    // emitter_data_.origin = Vector3(0, 0, 0);
    emitter_data_.velocity = Vector3(0, 0, 0);
    emitter_data_.particle_life_span = 10;
    emitter_data_.start_size = 0.03f;
    emitter_data_.end_size = 0.f;
    emitter_data_.mass = 1.f;
    emitter_data_.start_color = Vector4(1.0f, 0.2f, 0.2f, 1.f);
    emitter_data_.end_color = Vector4(0.0f, 0.0f, 0.0f, 0.f);
    emitter_data_.max_particles_this_frame = 10;
}

void ParticleSystem::draw()
{
    auto context = Game::inst()->render().context();

    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    context->OMGetRenderTargets(1, &rtv, &dsv);
    context->OMSetRenderTargets(0, nullptr, nullptr);

    uniform_buffer_.bind(0);

    context->OMSetDepthStencilState(use_depth_test_ ? depth_state_on_ : depth_state_off_, 0x0);
    context->RSSetState(rasterizer_state_);
    context->OMSetBlendState(blend_state_, nullptr, 0xFFFFFFFF);

    if (reset_flag_)
    {
        init_dead_list();

        {
            Annotation annotation("Reset");
            UINT initial_count = (UINT)-1;
            context->CSSetUnorderedAccessViews(0, 1, &particle_pool_UAV_, &initial_count);
            reset_.use();
            context->Dispatch(align(max_particles_count_, 256) / 256, 1, 1);
        }

        reset_flag_ = false;
    }

    emit();

    simulate();

    context->CopyStructureCount(sort_list_const_buffer_, 0, sort_list_UAV_);

#ifndef NDEBUG
    dead_particles_after_simulation_ = read_counter(dead_list_UAV_);
    sort_particles_after_simulation_ = read_counter(sort_list_UAV_);
    assert(sort_particles_after_simulation_ <= dead_particles_on_init_);
#endif

    sort();

    render_.use();
    ID3D11ShaderResourceView* vs_srv[] = { particle_pool_SRV_, sort_list_SRV_ };
    //ID3D11ShaderResourceView* ps_srv[] = { depth_view_ };
    ID3D11Buffer* vb = nullptr;
    UINT stride = 0;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->VSSetConstantBuffers(3, 1, &sort_list_const_buffer_);
    context->IASetIndexBuffer(index_buffer_, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShaderResources(0, ARRAYSIZE(vs_srv), vs_srv);
    //context->PSSetShaderResources(1, ARRAYSIZE(ps_srv), ps_srv);

    // Set the render target up since it was unbound earlier
    context->OMSetRenderTargets(1, &rtv, dsv);
    SAFE_RELEASE(rtv);
    SAFE_RELEASE(dsv);

    render_.use();

    context->DrawIndexedInstancedIndirect(indirect_args_, 0);

    ZeroMemory(vs_srv, sizeof(vs_srv));
    context->VSSetShaderResources(0, ARRAYSIZE(vs_srv), vs_srv);
    //ZeroMemory(ps_srv, sizeof(ps_srv));
    //context->PSSetShaderResources(1, ARRAYSIZE(ps_srv), ps_srv);
}

void ParticleSystem::imgui()
{
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();

    uint32_t window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_FirstUseEver);
    ImGui::Begin("Particle system info", nullptr, window_flags);
    {
#ifndef NDEBUG
        ImGui::Text("dead count on init");
        ImGui::SameLine();
        ImGui::Text("%d", dead_particles_on_init_);

        ImGui::Text("dead count after emit");
        ImGui::SameLine();
        ImGui::Text("%d", dead_particles_after_emit_);

        ImGui::Text("dead count after simulation");
        ImGui::SameLine();
        ImGui::Text("%d", dead_particles_after_simulation_);

        ImGui::Text("sort count after simulation");
        ImGui::SameLine();
        ImGui::Text("%d", sort_particles_after_simulation_);
#endif

        ImGui::Checkbox("collisions", &use_depth_for_collisions_);

        ImGui::Checkbox("depth test", &use_depth_test_);
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkSize.x - 400, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_FirstUseEver);
    ImGui::Begin("Emitter info", nullptr, window_flags);
    {
        ImGui::SliderInt("particles to emit per frame", &emitter_data_.max_particles_this_frame, 1, 100);

        ImGui::SliderFloat3("particle emit velocity", &emitter_data_.velocity.x, -10.f, 10.f);

        ImGui::SliderFloat("particle lifespan", &emitter_data_.particle_life_span, 0, 60);

        ImGui::SliderFloat("particle mass", &emitter_data_.mass, 0.f, 10.f);

        ImGui::SliderFloat("particle start size", &emitter_data_.start_size, 0.01f, 0.1f, "%.2f");
    }
    ImGui::End();
}

void ParticleSystem::update()
{
    auto camera = Game::inst()->render().camera();
    uniform_data_.view = camera->view();
    uniform_data_.view_inv = camera->view().Invert();
    uniform_data_.proj = camera->proj();
    uniform_data_.proj_inv = camera->proj().Invert();
    uniform_data_.view_proj = camera->view_proj();
    uniform_data_.view_proj_inv = camera->view_proj().Invert();

    uniform_data_.camera_position = camera->position();
    uniform_data_.camera_direction = camera->direction();

    uniform_data_.screen_width = Game::inst()->win().screen_width();
    uniform_data_.screen_height = Game::inst()->win().screen_height();
    uniform_data_.global_time += Game::inst()->delta_time();
    uniform_data_.frame_delta = Game::inst()->delta_time();
    uniform_buffer_.update_data(&uniform_data_);
}

void ParticleSystem::destroy_resources()
{
    uniform_buffer_.destroy();

    SAFE_RELEASE(particle_pool_SRV_);
    SAFE_RELEASE(particle_pool_UAV_);
    SAFE_RELEASE(particle_pool_);

    // SAFE_RELEASE(view_space_particle_positions_SRV_);
    // SAFE_RELEASE(view_space_particle_positions_UAV_);
    // SAFE_RELEASE(view_space_particle_positions_);

    SAFE_RELEASE(dead_list_UAV_);
    SAFE_RELEASE(dead_list_);

    SAFE_RELEASE(sort_list_SRV_);
    SAFE_RELEASE(sort_list_UAV_);
    SAFE_RELEASE(sort_list_);

    SAFE_RELEASE(index_buffer_);

    SAFE_RELEASE(indirect_args_UAV_);
    SAFE_RELEASE(indirect_args_);

#ifndef NDEBUG
    SAFE_RELEASE(debug_counter_buffer_);
#endif
    SAFE_RELEASE(dead_list_const_buffer_);
    SAFE_RELEASE(sort_list_const_buffer_);
    SAFE_RELEASE(emitter_const_buffer_);
    SAFE_RELEASE(blend_state_);
    SAFE_RELEASE(depth_state_on_);
    SAFE_RELEASE(depth_state_off_);
    SAFE_RELEASE(rasterizer_state_);

    init_dead_list_.destroy();
    emit_.destroy();
    simulate_.destroy();
    reset_.destroy();

    render_.destroy();

    sort_lib_.release();
}

void ParticleSystem::set_depth_shader_resource_view(ID3D11ShaderResourceView* depth_view, ID3D11ShaderResourceView* normal_view)
{
    depth_view_ = depth_view;
    normal_view_ = normal_view;
}

void ParticleSystem::init_dead_list()
{
    Annotation annotation("Init dead list");
    auto context = Game::inst()->render().context();

    init_dead_list_.use();
    UINT initial_count = 0;
    context->CSSetUnorderedAccessViews(0, 1, &dead_list_UAV_, &initial_count);

    context->Dispatch(align(max_particles_count_, 256) / 256, 1, 1);

#ifndef NDEBUG
    dead_particles_on_init_ = read_counter(dead_list_UAV_);
#endif
}

void ParticleSystem::emit()
{
    Annotation annotation("Emit");
    auto context = Game::inst()->render().context();

    ID3D11UnorderedAccessView* uavs[] = { particle_pool_UAV_, dead_list_UAV_ };
    UINT initialCounts[] = { (UINT)-1, (UINT)-1 };
    context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, initialCounts);

    ID3D11Buffer* buffers[] = { emitter_const_buffer_, dead_list_const_buffer_ };
    context->CSSetConstantBuffers(1, ARRAYSIZE(buffers), buffers);

    emit_.use();
    D3D11_MAPPED_SUBRESOURCE mss;
    context->Map(emitter_const_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mss);
    memcpy(mss.pData, &emitter_data_, sizeof(EmitterParams));
    context->Unmap(emitter_const_buffer_, 0);

    context->CopyStructureCount(dead_list_const_buffer_, 0, dead_list_UAV_);

    context->Dispatch(align(emitter_data_.max_particles_this_frame, 1024) / 1024, 1, 1);

#ifndef NDEBUG
    dead_particles_after_emit_ = read_counter(dead_list_UAV_);
#endif
}

void ParticleSystem::simulate()
{
    Annotation annotation("Simulate");
    auto context = Game::inst()->render().context();

    uniform_buffer_.bind(0);

    ID3D11UnorderedAccessView* uavs[] = { particle_pool_UAV_, dead_list_UAV_, sort_list_UAV_, indirect_args_UAV_ };
    UINT initialCounts[] = { (UINT)-1, (UINT)-1, (UINT)0, (UINT)-1, (UINT)-1, (UINT)-1 };
    context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, initialCounts);

    ID3D11ShaderResourceView* srvs[] = { depth_view_, normal_view_ };
    if (use_depth_for_collisions_) {
        context->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);
    }

    simulate_.use();
    context->Dispatch(align(max_particles_count_, 256) / 256, 1, 1);

    if (use_depth_for_collisions_) {
        ZeroMemory(srvs, sizeof(srvs));
        context->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);
    }

    ZeroMemory(uavs, sizeof(uavs));
    context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, nullptr);
}

void ParticleSystem::sort()
{
    Annotation annotation("Sort");
    auto context = Game::inst()->render().context();

    sort_lib_.run(max_particles_count_, sort_list_UAV_, sort_list_const_buffer_);
}

#ifndef NDEBUG
int ParticleSystem::read_counter(ID3D11UnorderedAccessView* uav)
{
    auto context = Game::inst()->render().context();
    int count = 0;

    // Copy the UAV counter to a staging resource
    context->CopyStructureCount(debug_counter_buffer_, 0, uav);

    // Map the staging resource
    D3D11_MAPPED_SUBRESOURCE mss;
    context->Map(debug_counter_buffer_, 0, D3D11_MAP_READ, 0, &mss);

    // Read the data
    count = *(int*)mss.pData;

    context->Unmap(debug_counter_buffer_, 0);

    return count;
}
#endif
