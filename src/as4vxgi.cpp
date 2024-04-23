#include "render/common.h"

#include "core/game.h"
#include "render/render.h"
#include "render/camera.h"

#include "as4vxgi.h"

#include <imgui/imgui.h>

int32_t voxel_grid_dim = 100;
float voxel_grid_size = 500;

inline int align(int value, int alignment)
{
    return (value + (alignment - 1)) & ~(alignment - 1);
}

void AS4VXGI_Component::initialize()
{
    Game::inst()->render().camera()->set_camera(Vector3(5, 0, 0), Vector3(-1, 0, 0));

    constexpr float offset = 300;
    constexpr float count = 10;
    for (int x = 0; x < count; ++x) {
        for (int y = 0; y < count; ++y) {
            model_trees_.push_back(new ModelTree{});
            model_trees_.back()->load("./resources/models/suzanne.fbx", Vector3((x - count / 2) * offset, 0, (y - count / 2) * offset));
        }
    }

    // model_trees_.push_back(new ModelTree{});
    // model_trees_.back()->load("./resources/models/suzanne.fbx");//, Vector3(0, 0, 150));

    //model_trees_.push_back(new ModelTree{});
    //model_trees_.back()->load("./resources/models/suzanne.fbx", Vector3(0, 0, -150));
    //model_trees_.push_back(new ModelTree{});
    //model_trees_.back()->load("./resources/models/terrain.fbx");
    //model_trees_.back()->load("./resources/models/sponza/source/sponza.fbx");

    auto device = Game::inst()->render().device();

    {
        // voxels fill pass
        {
            voxels_fill_.attach_compute_shader(L"./resources/shaders/voxels/fill.hlsl", {});
            voxels_fill_.declare_bind<CAMERA_DATA_BIND>();
            voxels_fill_.declare_bind<MESH_TREE_BIND>();
            voxels_fill_.declare_bind<INDICES_BIND>();
            voxels_fill_.declare_bind<VERTICES_BIND>();
            voxels_fill_.declare_bind<MODEL_MATRICES_BIND>();
            voxels_fill_.declare_bind<VOXEL_DATA_BIND>();
            voxels_fill_.declare_bind<VOXELS_BIND>();
            voxels_fill_.create_pso_and_root_signature();
        }
        // voxels vizualize pass
        {
            D3D12_INPUT_ELEMENT_DESC inputs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };
            stage_visualize_pipeline_.setup_input_layout(inputs, _countof(inputs));
            stage_visualize_pipeline_.attach_vertex_shader(L"./resources/shaders/voxels/draw.hlsl", {});
            stage_visualize_pipeline_.attach_geometry_shader(L"./resources/shaders/voxels/draw.hlsl", {});
            stage_visualize_pipeline_.attach_pixel_shader(L"./resources/shaders/voxels/draw.hlsl", {});

            stage_visualize_pipeline_.declare_bind<CAMERA_DATA_BIND>();
            stage_visualize_pipeline_.declare_bind<VOXELS_BIND>();
            stage_visualize_pipeline_.declare_bind<VOXEL_DATA_BIND>();

            CD3DX12_DEPTH_STENCIL_DESC ds_state(D3D12_DEFAULT);
            stage_visualize_pipeline_.setup_depth_stencil_state(ds_state);

            stage_visualize_pipeline_.setup_primitive_topology_type(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);

            stage_visualize_pipeline_.create_pso_and_root_signature();
        }
    }

    // create voxels uavs
    UINT voxels_uav_size = voxel_grid_dim * voxel_grid_dim * voxel_grid_dim * sizeof(Voxel);
    {
        HRESULT_CHECK(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex3D(DXGI_FORMAT_R32G32B32A32_FLOAT,
                voxel_grid_dim, voxel_grid_dim, voxel_grid_dim, 0,
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
            D3D12_RESOURCE_STATE_COMMON, nullptr,
            IID_PPV_ARGS(uav_voxels_resource_.GetAddressOf())));
        uav_voxels_resource_->SetName(L"Render voxels resource");

        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            uav_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            uav_desc.Texture3D.FirstWSlice= 0;
            uav_desc.Texture3D.MipSlice = 0;
            uav_desc.Texture3D.WSize = -1;

            uav_voxels_resource_index_ = Game::inst()->render().allocate_gpu_resource_descriptor(uav_voxels_, uav_voxels_gpu_);
            device->CreateUnorderedAccessView(uav_voxels_resource_.Get(), nullptr, &uav_desc, uav_voxels_);

            uav_voxels_resource_index_cpu_ = Game::inst()->render().allocate_cpu_resource_descriptor(uav_voxels_cpu_);
            device->CreateUnorderedAccessView(uav_voxels_resource_.Get(), nullptr, &uav_desc, uav_voxels_cpu_);
        }

        ComPtr<ID3D12GraphicsCommandList> barrier;
        device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, Game::inst()->render().graphics_command_allocator().Get(), nullptr, IID_PPV_ARGS(&barrier));
        barrier->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(uav_voxels_resource_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        barrier->Close();
        Game::inst()->render().graphics_queue()->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(barrier.GetAddressOf()));
    }

    // fill voxels
    for (int32_t i = 0; i < model_trees_.size(); ++i) {
        ModelTree* model_tree = model_trees_[i];
        Matrix transform = model_tree->get_transform();

        index_buffers_srv_.push_back({});
        for (D3D12_GPU_DESCRIPTOR_HANDLE srv : model_tree->get_index_buffers_srv()) {
            index_buffers_srv_[i].push_back(srv);
        }
        vertex_buffers_srv_.push_back({});
        for (D3D12_GPU_DESCRIPTOR_HANDLE srv : model_tree->get_vertex_buffers_srv()) {
            vertex_buffers_srv_[i].push_back(srv);
        }
        mesh_trees_srv_.push_back({});
        model_matrix_srv_.push_back({});
        for (std::vector<MeshTreeNode> mesh_tree : model_tree->get_meshes_trees()) {
            mesh_trees_srv_[i].push_back(new ShaderResource<MeshTreeNode>());
            mesh_trees_srv_[i].back()->initialize(mesh_tree.data(), UINT(mesh_tree.size()));

            model_matrix_srv_[i].push_back({});
            model_matrix_srv_[i].back() = new ShaderResource<Matrix>();
            model_matrix_srv_[i].back()->initialize(&transform, 1);
        }
    }
    voxel_data_.voxelGrid.dimension = voxel_grid_dim;
    voxel_data_.voxelGrid.size = voxel_grid_size;
    voxel_data_.voxelGrid.mesh_node_count = 0;

    // create const buffer view
    {
        voxel_data_cb_.initialize();
        voxel_data_cb_.update(voxel_data_);
    }
}

void AS4VXGI_Component::draw()
{
    auto device = Game::inst()->render().device();
    auto graphics_command_list_allocator = Game::inst()->render().graphics_command_allocator();
    auto resource_descriptor_heap = Game::inst()->render().resource_descriptor_heap();
    const auto render_target = Game::inst()->render().render_target();
    const auto depth_stencil_target = Game::inst()->render().depth_stencil();
    auto graphics_queue = Game::inst()->render().graphics_queue();

    {
        ComPtr<ID3D12GraphicsCommandList> cmd;
        HRESULT_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, graphics_command_list_allocator.Get(), nullptr, IID_PPV_ARGS(cmd.ReleaseAndGetAddressOf())));
        cmd->SetName(L"Voxels command list");
        HRESULT_CHECK(cmd->Close());
        HRESULT_CHECK(cmd->Reset(graphics_command_list_allocator.Get(), nullptr));
        {
            PIXBeginEvent(cmd.Get(), PIX_COLOR(0xFF, 0x0, 0x0), "Voxels clear");
            {
                FLOAT clear[4] = {0, 0, 0, 0};
                cmd->SetDescriptorHeaps(1, resource_descriptor_heap.GetAddressOf());
                cmd->ClearUnorderedAccessViewFloat(uav_voxels_gpu_, uav_voxels_cpu_, uav_voxels_resource_.Get(), clear, 0, nullptr);
            }
            PIXEndEvent(cmd.Get());

            cmd->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(uav_voxels_resource_.Get()));

            PIXBeginEvent(cmd.Get(), PIX_COLOR(0xFF, 0x0, 0x0), "Voxels fill");
            {
                for (int i = 0; i < mesh_trees_srv_.size(); ++i) {
                    for (int32_t j = 0; j < mesh_trees_srv_[i].size(); ++j) {
                        cmd->SetPipelineState(voxels_fill_.get_pso());
                        cmd->SetComputeRootSignature(voxels_fill_.get_root_signature());
                        cmd->SetDescriptorHeaps(1, resource_descriptor_heap.GetAddressOf());

                        cmd->SetComputeRootDescriptorTable(voxels_fill_.resource_index<CAMERA_DATA_BIND>(), Game::inst()->render().camera()->gpu_descriptor_handle());
                        cmd->SetComputeRootDescriptorTable(voxels_fill_.resource_index<VOXEL_DATA_BIND>(), voxel_data_cb_.gpu_descriptor_handle());
                        cmd->SetComputeRootDescriptorTable(voxels_fill_.resource_index<VOXELS_BIND>(), uav_voxels_gpu_);

                        voxel_data_.voxelGrid.mesh_node_count = static_cast<int32_t>(mesh_trees_srv_[i][j]->size());
                        voxel_data_cb_.update(voxel_data_);

                        cmd->SetComputeRootDescriptorTable(voxels_fill_.resource_index<MESH_TREE_BIND>(), mesh_trees_srv_[i][j]->gpu_descriptor_handle());
                        cmd->SetComputeRootDescriptorTable(voxels_fill_.resource_index<INDICES_BIND>(), index_buffers_srv_[i][j]);
                        cmd->SetComputeRootDescriptorTable(voxels_fill_.resource_index<VERTICES_BIND>(), vertex_buffers_srv_[i][j]);
                        cmd->SetComputeRootDescriptorTable(voxels_fill_.resource_index<MODEL_MATRICES_BIND>(), model_matrix_srv_[i][j]->gpu_descriptor_handle());

                        cmd->Dispatch(align(voxel_grid_dim, 4) / 4,
                            align(voxel_grid_dim, 4) / 4,
                            align(voxel_grid_dim, 4) / 4);
                    }
                }
            }
            PIXEndEvent(cmd.Get());

            cmd->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(uav_voxels_resource_.Get()));

            PIXBeginEvent(cmd.Get(), PIX_COLOR(0x0, 0xFF, 0x0), "Voxels draw");
            {
                cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
                cmd->RSSetViewports(1, &Game::inst()->render().viewport());
                cmd->RSSetScissorRects(1, &Game::inst()->render().scissor_rect());
                cmd->OMSetRenderTargets(1, &render_target, 1, &depth_stencil_target);

                cmd->SetPipelineState(stage_visualize_pipeline_.get_pso());
                cmd->SetGraphicsRootSignature(stage_visualize_pipeline_.get_root_signature());
                cmd->SetDescriptorHeaps(1, resource_descriptor_heap.GetAddressOf());
                cmd->SetGraphicsRootDescriptorTable(stage_visualize_pipeline_.resource_index<CAMERA_DATA_BIND>(), Game::inst()->render().camera()->gpu_descriptor_handle());
                cmd->SetGraphicsRootDescriptorTable(stage_visualize_pipeline_.resource_index<VOXELS_BIND>(), uav_voxels_gpu_);
                cmd->SetGraphicsRootDescriptorTable(stage_visualize_pipeline_.resource_index<VOXEL_DATA_BIND>(), voxel_data_cb_.gpu_descriptor_handle());

                cmd->DrawInstanced(1, voxel_grid_dim * voxel_grid_dim * voxel_grid_dim, 0, 0);
            }
            PIXEndEvent(cmd.Get());
        }

        HRESULT_CHECK(cmd->Close());
        graphics_queue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(cmd.GetAddressOf()));
    }

    ComPtr<ID3D12GraphicsCommandList> draw_cmd_list;
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, graphics_command_list_allocator.Get(), nullptr, IID_PPV_ARGS(&draw_cmd_list));
    HRESULT_CHECK(draw_cmd_list->Close());
    HRESULT_CHECK(draw_cmd_list->Reset(graphics_command_list_allocator.Get(), nullptr));

    for (ModelTree* model_tree : model_trees_) {
        model_tree->draw(draw_cmd_list.Get());
    }

    HRESULT_CHECK(draw_cmd_list->Close());
    graphics_queue->ExecuteCommandLists(1, (ID3D12CommandList**)draw_cmd_list.GetAddressOf());
}

void AS4VXGI_Component::imgui()
{
#ifndef NDEBUG
    // ImGuiViewport* main_viewport = ImGui::GetMainViewport();

    uint32_t window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 100), ImGuiCond_FirstUseEver);
    float local_voxel_grid_size = voxel_grid_size;
    int local_voxel_grid_dim = voxel_grid_dim;
    ImGui::Begin("info", nullptr, window_flags);
    {
        ImGui::Text("Voxel grid size");
        ImGui::SameLine();
        ImGui::InputFloat("##local_voxel_grid_size", &local_voxel_grid_size, 0.01f, 1.f);

        ImGui::Text("Voxel grid dimensions");
        ImGui::SameLine();
        ImGui::InputInt("##local_voxel_grid_dim", &local_voxel_grid_dim, 1, 10);
    }
    ImGui::End();

    if (false && local_voxel_grid_dim > 0 && local_voxel_grid_size > 0) {
        /// TODO: handle all uavs recreation
        if (local_voxel_grid_dim != voxel_grid_dim) {
            uav_voxels_resource_.Reset();
            UINT voxels_uav_size = local_voxel_grid_dim * local_voxel_grid_dim * local_voxel_grid_dim * sizeof(Voxel);
            auto device = Game::inst()->render().device();

            HRESULT_CHECK(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(voxels_uav_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
                D3D12_RESOURCE_STATE_COMMON, nullptr,
                IID_PPV_ARGS(uav_voxels_resource_.ReleaseAndGetAddressOf())));

            D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uav_desc.Format = DXGI_FORMAT_UNKNOWN;
            uav_desc.Buffer.CounterOffsetInBytes = 0;
            uav_desc.Buffer.NumElements = local_voxel_grid_dim * local_voxel_grid_dim * local_voxel_grid_dim;
            uav_desc.Buffer.StructureByteStride = sizeof(Voxel);
            uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

            device->CreateUnorderedAccessView(uav_voxels_resource_.Get(), nullptr, &uav_desc, uav_voxels_);
        }

        if (local_voxel_grid_size != voxel_grid_size || local_voxel_grid_dim != voxel_grid_dim) {
            voxel_grid_size = local_voxel_grid_size;
            voxel_grid_dim = local_voxel_grid_dim;

            voxel_data_.voxelGrid.dimension = voxel_grid_dim;
            voxel_data_.voxelGrid.size = voxel_grid_size;
            voxel_data_.voxelGrid.mesh_node_count = 0;
            voxel_data_cb_.update(voxel_data_);
        }
    }
#endif
}

void AS4VXGI_Component::reload()
{

}

void AS4VXGI_Component::update()
{

}

void AS4VXGI_Component::destroy_resources()
{
    model_matrix_srv_.clear();
    mesh_trees_srv_.clear();

    for (ModelTree* model_tree : model_trees_) {
        model_tree->unload();
        delete model_tree;
    }

    model_trees_.clear();

    uav_voxels_resource_.Reset();
}
