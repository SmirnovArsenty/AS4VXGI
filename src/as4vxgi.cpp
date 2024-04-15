#include "render/common.h"

#include <imgui/imgui.h>

#include "core/game.h"
#include "render/render.h"
#include "render/camera.h"

#include "as4vxgi.h"

int32_t voxel_grid_dim = 30;
float voxel_grid_size = 1000;

inline int align(int value, int alignment)
{
    return (value + (alignment - 1)) & ~(alignment - 1);
}

void AS4VXGI_Component::initialize()
{
    Game::inst()->render().camera()->set_camera(Vector3(5, 0, 0), Vector3(-1, 0, 0));

    // constexpr uint32_t offset = 300;
    // constexpr uint32_t count = 10;
    // for (int x = 0; x < count; ++x) {
    //     for (int y = 0; y < count; ++y) {
    //         model_trees_.push_back(new ModelTree{});
    //         model_trees_.back()->load("./resources/models/suzanne.fbx", Vector3((x - count / 2) * offset, 0, (y - count / 2) * offset));
    //     }
    // }

    model_trees_.push_back(new ModelTree{});
    model_trees_.back()->load("./resources/models/suzanne.fbx");//, Vector3(0, 0, 150));

    //model_trees_.push_back(new ModelTree{});
    //model_trees_.back()->load("./resources/models/suzanne.fbx", Vector3(0, 0, -150));
    //model_trees_.push_back(new ModelTree{});
    //model_trees_.back()->load("./resources/models/terrain.fbx");
    //model.load("./resources/models/sponza/source/sponza.fbx");

    auto device = Game::inst()->render().device();

    {
        // create command lists

        if (true)
        {
            stage_1_pipeline_ = new ComputePipeline();
            stage_1_pipeline_->attach_compute_shader(L"./resources/shaders/voxels/clear.hlsl", {});

            // space 1
            stage_1_pipeline_->declare_bind<VOXELS_BIND>();

            stage_1_pipeline_->create_command_list();
        }
        if (true)
        {
            stage_2_pipeline_ = new ComputePipeline();
            stage_2_pipeline_->attach_compute_shader(L"./resources/shaders/voxels/fill.hlsl", {});

            // space 0
            stage_2_pipeline_->declare_bind<CAMERA_DATA_BIND>();
            stage_2_pipeline_->declare_bind<MESH_TREE_BIND>();
            stage_2_pipeline_->declare_bind<INDICES_BIND>();
            stage_2_pipeline_->declare_bind<VERTICES_BIND>();
            stage_2_pipeline_->declare_bind<MODEL_MATRICES_BIND>();

            // space 1
            stage_2_pipeline_->declare_bind<VOXEL_DATA_BIND>();
            stage_2_pipeline_->declare_bind<VOXELS_BIND>();

            stage_2_pipeline_->create_command_list();
        }
        if (true)
        {
            stage_visualize_pipeline_ = new GraphicsPipeline();
            D3D12_INPUT_ELEMENT_DESC inputs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };
            stage_visualize_pipeline_->setup_input_layout(inputs, _countof(inputs));
            stage_visualize_pipeline_->attach_vertex_shader(L"./resources/shaders/voxels/draw.hlsl", {});
            stage_visualize_pipeline_->attach_geometry_shader(L"./resources/shaders/voxels/draw.hlsl", {});
            stage_visualize_pipeline_->attach_pixel_shader(L"./resources/shaders/voxels/draw.hlsl", {});

            // space 0
            stage_visualize_pipeline_->declare_bind<CAMERA_DATA_BIND>();

            // space 1
            stage_visualize_pipeline_->declare_bind<VOXELS_BIND>();
            stage_visualize_pipeline_->declare_bind<VOXEL_DATA_BIND>();

            CD3DX12_DEPTH_STENCIL_DESC ds_state(D3D12_DEFAULT);
            stage_visualize_pipeline_->setup_depth_stencil_state(ds_state);

            stage_visualize_pipeline_->setup_primitive_topology_type(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);

            stage_visualize_pipeline_->create_command_list();
        }
    }

    // create voxels uav
    {
        UINT voxels_uav_size = voxel_grid_dim * voxel_grid_dim * voxel_grid_dim * sizeof(Voxel);
        HRESULT_CHECK(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(voxels_uav_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
            D3D12_RESOURCE_STATE_COMMON, nullptr,
            IID_PPV_ARGS(uav_voxels_resource_.GetAddressOf())));

        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uav_desc.Format = DXGI_FORMAT_UNKNOWN;
            uav_desc.Buffer.CounterOffsetInBytes = 0;
            uav_desc.Buffer.NumElements = voxel_grid_dim * voxel_grid_dim * voxel_grid_dim;
            uav_desc.Buffer.StructureByteStride = sizeof(Voxel);
            uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

            uav_voxels_resource_index_ = Game::inst()->render().allocate_resource_descriptor(uav_voxels_, uav_voxels_gpu_);
            device->CreateUnorderedAccessView(uav_voxels_resource_.Get(), nullptr, &uav_desc, uav_voxels_);
        }
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
    auto graphics_command_list_allocator = Game::inst()->render().graphics_command_allocator();
    auto resource_descriptor_heap = Game::inst()->render().resource_descriptor_heap();
    const auto render_target = Game::inst()->render().render_target();
    const auto depth_stencil_target = Game::inst()->render().depth_stencil();
    auto graphics_queue = Game::inst()->render().graphics_queue();
    if (true)
    {
        // clear voxel grid
        {
            // stage 1
            stage_1_pipeline_->add_cmd()->Reset(graphics_command_list_allocator.Get(), stage_1_pipeline_->get_pso());
            PIXBeginEvent(stage_1_pipeline_->add_cmd(), 0xFF00FFFF, "Voxels clear");

            stage_1_pipeline_->add_cmd()->SetComputeRootSignature(stage_1_pipeline_->get_root_signature());
            stage_1_pipeline_->add_cmd()->SetDescriptorHeaps(1, resource_descriptor_heap.GetAddressOf());
            stage_1_pipeline_->add_cmd()->SetComputeRootDescriptorTable(stage_1_pipeline_->resource_index<VOXELS_BIND>(), uav_voxels_gpu_);

            stage_1_pipeline_->add_cmd()->Dispatch(align(voxel_grid_dim * voxel_grid_dim * voxel_grid_dim, 256) / 256, 1, 1);

            stage_1_pipeline_->add_cmd()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(uav_voxels_resource_.Get()));

            PIXEndEvent(stage_1_pipeline_->add_cmd());
            HRESULT_CHECK(stage_1_pipeline_->add_cmd()->Close());
        }

        // fill voxels grid params
        {
            // stage 2
            stage_2_pipeline_->add_cmd()->Reset(graphics_command_list_allocator.Get(), stage_2_pipeline_->get_pso());
            PIXBeginEvent(stage_2_pipeline_->add_cmd(), 0xFF00FFFF, "Voxels fill");

            stage_2_pipeline_->add_cmd()->SetComputeRootSignature(stage_2_pipeline_->get_root_signature());
            stage_2_pipeline_->add_cmd()->SetDescriptorHeaps(1, resource_descriptor_heap.GetAddressOf());
            stage_2_pipeline_->add_cmd()->SetComputeRootDescriptorTable(stage_2_pipeline_->resource_index<CAMERA_DATA_BIND>(), Game::inst()->render().camera()->gpu_descriptor_handle());

            stage_2_pipeline_->add_cmd()->SetComputeRootDescriptorTable(stage_2_pipeline_->resource_index<VOXEL_DATA_BIND>(), voxel_data_cb_.gpu_descriptor_handle());
            stage_2_pipeline_->add_cmd()->SetComputeRootDescriptorTable(stage_2_pipeline_->resource_index<VOXELS_BIND>(), uav_voxels_gpu_);

            for (int i = 0; i < mesh_trees_srv_.size(); ++i) {
                for (int32_t j = 0; j < mesh_trees_srv_[i].size(); ++j) {
                    voxel_data_.voxelGrid.mesh_node_count = static_cast<int32_t>(mesh_trees_srv_[i][j]->size());
                    voxel_data_cb_.update(voxel_data_);

                    stage_2_pipeline_->add_cmd()->SetComputeRootDescriptorTable(stage_2_pipeline_->resource_index<MESH_TREE_BIND>(), mesh_trees_srv_[i][j]->gpu_descriptor_handle());
                    stage_2_pipeline_->add_cmd()->SetComputeRootDescriptorTable(stage_2_pipeline_->resource_index<INDICES_BIND>(), index_buffers_srv_[i][j]);
                    stage_2_pipeline_->add_cmd()->SetComputeRootDescriptorTable(stage_2_pipeline_->resource_index<VERTICES_BIND>(), vertex_buffers_srv_[i][j]);
                    stage_2_pipeline_->add_cmd()->SetComputeRootDescriptorTable(stage_2_pipeline_->resource_index<MODEL_MATRICES_BIND>(), model_matrix_srv_[i][j]->gpu_descriptor_handle());

                    stage_2_pipeline_->add_cmd()->Dispatch(align(voxel_grid_dim, 4) / 4,
                                                            align(voxel_grid_dim, 4) / 4,
                                                            align(voxel_grid_dim, 4) / 4);


                }
            }
            stage_2_pipeline_->add_cmd()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(uav_voxels_resource_.Get()));

            PIXEndEvent(stage_2_pipeline_->add_cmd());
            HRESULT_CHECK(stage_2_pipeline_->add_cmd()->Close());
        }

        // visualize voxel grid
        {
            stage_visualize_pipeline_->add_cmd()->Reset(graphics_command_list_allocator.Get(), stage_visualize_pipeline_->get_pso());
            PIXBeginEvent(stage_visualize_pipeline_->add_cmd(), 0xFF00FFFF, "Voxels draw");

            stage_visualize_pipeline_->add_cmd()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
            stage_visualize_pipeline_->add_cmd()->RSSetViewports(1, &Game::inst()->render().viewport());
            stage_visualize_pipeline_->add_cmd()->RSSetScissorRects(1, &Game::inst()->render().scissor_rect());
            stage_visualize_pipeline_->add_cmd()->OMSetRenderTargets(1, &render_target, 1, &depth_stencil_target);

            stage_visualize_pipeline_->add_cmd()->SetGraphicsRootSignature(stage_visualize_pipeline_->get_root_signature());
            stage_visualize_pipeline_->add_cmd()->SetDescriptorHeaps(1, resource_descriptor_heap.GetAddressOf());
            stage_visualize_pipeline_->add_cmd()->SetGraphicsRootDescriptorTable(stage_visualize_pipeline_->resource_index<CAMERA_DATA_BIND>(), Game::inst()->render().camera()->gpu_descriptor_handle());
            stage_visualize_pipeline_->add_cmd()->SetGraphicsRootDescriptorTable(stage_visualize_pipeline_->resource_index<VOXELS_BIND>(), uav_voxels_gpu_);
            stage_visualize_pipeline_->add_cmd()->SetGraphicsRootDescriptorTable(stage_visualize_pipeline_->resource_index<VOXEL_DATA_BIND>(), voxel_data_cb_.gpu_descriptor_handle());

            stage_visualize_pipeline_->add_cmd()->DrawInstanced(1, voxel_grid_dim * voxel_grid_dim * voxel_grid_dim, 0, 0);

            PIXEndEvent(stage_visualize_pipeline_->add_cmd());
            HRESULT_CHECK(stage_visualize_pipeline_->add_cmd()->Close());
        }

        ID3D12CommandList* lists[] = {stage_1_pipeline_->add_cmd(), stage_2_pipeline_->add_cmd(), stage_visualize_pipeline_->add_cmd()};
        graphics_queue->ExecuteCommandLists(_countof(lists), lists);
    }

    for (ModelTree* model_tree : model_trees_) {
        model_tree->draw(Game::inst()->render().camera());
    }
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

    if (local_voxel_grid_dim > 0 && local_voxel_grid_size > 0) {
        if (local_voxel_grid_dim != voxel_grid_dim) {
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
    delete stage_visualize_pipeline_;
    stage_visualize_pipeline_ = nullptr;

    delete stage_2_pipeline_;
    stage_2_pipeline_ = nullptr;

    delete stage_1_pipeline_;
    stage_1_pipeline_ = nullptr;

    model_matrix_srv_.clear();
    mesh_trees_srv_.clear();

    for (ModelTree* model_tree : model_trees_) {
        model_tree->unload();
        delete model_tree;
    }

    model_trees_.clear();

    SAFE_RELEASE(uav_voxels_resource_);
}
