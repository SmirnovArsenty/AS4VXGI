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
    Game::inst()->render().camera()->set_camera(Vector3(500, 0, 0), Vector3(-1, 0, 0));

    constexpr uint32_t offset = 300;
    constexpr uint32_t count = 10;
    //for (int x = 0; x < count; ++x) {
    //    for (int y = 0; y < count; ++y) {
    //        model_trees_.push_back(new ModelTree{});
    //        model_trees_.back()->load("./resources/models/suzanne.fbx", Vector3((x) * offset, 0, (y) * offset));
    //    }
    //}
    model_trees_.push_back(new ModelTree{});
    model_trees_.back()->load("./resources/models/suzanne.fbx");//, Vector3(0, 0, 150));
    //model_trees_.push_back(new ModelTree{});
    //model_trees_.back()->load("./resources/models/suzanne.fbx", Vector3(0, 0, -150));
    //model_trees_.push_back(new ModelTree{});
    //model_trees_.back()->load("./resources/models/terrain.fbx");
    //model.load("./resources/models/sponza/source/sponza.fbx");

    auto& device = Game::inst()->render().device();
    auto& descriptor_heap = Game::inst()->render().resource_descriptor_heap();

    {
        // create command lists

        if (1)
        {
            stage_1_pipeline_ = new ComputePipeline();
            stage_1_pipeline_->attach_compute_shader(L"./resources/shaders/voxels/clear.hlsl", {});

            stage_1_pipeline_->declare_bind<VOXELS_BIND>();

            stage_1_pipeline_->create_command_list();
        }
        if (1)
        {
            stage_2_pipeline_ = new ComputePipeline();
            stage_2_pipeline_->attach_compute_shader(L"./resources/shaders/voxels/fill.hlsl", {});

            stage_2_pipeline_->declare_bind<COMMON_BIND>();
            stage_2_pipeline_->declare_bind<VOXELS_BIND>();
            stage_2_pipeline_->declare_bind<MESH_TREE_BIND>();
            stage_2_pipeline_->declare_bind<INDICES_BIND>();
            stage_2_pipeline_->declare_bind<VERTICES_BIND>();
            stage_2_pipeline_->declare_bind<MODEL_MATRICES_BIND>();

            stage_2_pipeline_->create_command_list();
        }
        if (1)
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

            stage_visualize_pipeline_->declare_bind<COMMON_BIND>();
            stage_visualize_pipeline_->declare_bind<VOXELS_BIND>();

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

        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.Buffer.CounterOffsetInBytes = 0;
        uav_desc.Buffer.NumElements = voxel_grid_dim * voxel_grid_dim * voxel_grid_dim;
        uav_desc.Buffer.StructureByteStride = sizeof(Voxel);
        uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        uav_voxels_resource_index_ = Game::inst()->render().allocate_resource_descriptor(uav_voxels_);
        device->CreateUnorderedAccessView(uav_voxels_resource_.Get(), nullptr, &uav_desc, uav_voxels_);
    }

    // fill voxels
    for (int32_t i = 0; i < model_trees_.size(); ++i) {
        ModelTree* model_tree = model_trees_[i];
        Matrix transform = model_tree->get_transform();

        // index_buffers_srv_.push_back({});
        //for (ID3D11ShaderResourceView* srv : model_tree->get_index_buffers_srv()) {
        //    index_buffers_srv_[i].push_back(srv);
        //}
        //vertex_buffers_srv_.push_back({});
        //for (ID3D11ShaderResourceView* srv : model_tree->get_vertex_buffers_srv()) {
        //    vertex_buffers_srv_[i].push_back(srv);
        //}
        //mesh_trees_.push_back({});
        //model_matrix_srv_.push_back({});
        //for (std::vector<MeshTreeNode> mesh_tree : model_tree->get_meshes_trees()) {
        //    mesh_trees_[i].push_back(new ShaderResource<MeshTreeNode>());
        //    mesh_trees_[i].back()->initialize(mesh_tree.data(), mesh_tree.size());
        //
        //    model_matrix_srv_[i].push_back({nullptr});
        //    model_matrix_srv_[i].back() = new ShaderResource<Matrix>();
        //    model_matrix_srv_[i].back()->initialize(&transform, 1);
        //}
    }
    common_.voxelGrid.dimension = voxel_grid_dim;
    common_.voxelGrid.size = voxel_grid_size;
    common_.voxelGrid.mesh_node_count = 0;

    // create const buffer view
    {
        common_cb_.initialize();
        common_cb_.update(common_);
    }
}

void AS4VXGI_Component::draw()
{
    if (false)
    {
        // clear voxel grid
        {
            // stage 1
            stage_1_pipeline_->add_cmd()->Dispatch(align(voxel_grid_dim * voxel_grid_dim * voxel_grid_dim, 256) / 256, 1, 1);
        }

        // fill voxels grid params
        {
            // stage 2
            // shader_voxels_fill_.use();
            // for (int i = 0; i < mesh_trees_.size(); ++i) {
            //     for (int32_t j = 0; j < mesh_trees_[i].size(); ++j) {
            // 
            //         voxel_grid_params_.mesh_node_count = static_cast<int32_t>(mesh_trees_[i][j]->size());
            //         voxel_grid_params_buffer_.update(&voxel_grid_params_);
            //         context->CSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());
            // 
            //         context->CSSetShaderResources(0, 1, &mesh_trees_[i][j]->getSRV());
            //         context->CSSetShaderResources(1, 1, &index_buffers_srv_[i][j]);
            //         context->CSSetShaderResources(2, 1, &vertex_buffers_srv_[i][j]);
            //         context->CSSetShaderResources(3, 1, &model_matrix_srv_[i][j]->getSRV());
            // 
                    stage_2_pipeline_->add_cmd()->Dispatch(align(voxel_grid_dim, 4) / 4,
                                                            align(voxel_grid_dim, 4) / 4,
                                                            align(voxel_grid_dim, 4) / 4);
            // 
            //         context->CSSetUnorderedAccessViews(0, 1, &unbind_uav_.getUAV(), nullptr); // unbind voxels from uav to use it as srv
            //     }
            // }

            // visualize voxel grid
            {
                // Annotation debug("voxels debug");
                // shader_voxels_draw_.use();
                // context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
                // 
                // context->VSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());
                // context->HSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());
                // context->DSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());
                // context->GSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());
                // context->PSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());
                // 
                // context->VSSetShaderResources(0, 1, &voxels_.getSRV());

                stage_visualize_pipeline_->add_cmd()->DrawInstanced(1, voxel_grid_dim * voxel_grid_dim * voxel_grid_dim, 0, 0);
            }
        }
    }

    for (ModelTree* model_tree : model_trees_) {
        model_tree->draw(Game::inst()->render().camera());
    }
}

void AS4VXGI_Component::imgui()
{
#ifndef NDEBUG
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();

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

        ImGui::Text("Voxel grid dimentions");
        ImGui::SameLine();
        ImGui::InputInt("##local_voxel_grid_dim", &local_voxel_grid_dim, 1, 10);
    }
    ImGui::End();

    if (local_voxel_grid_dim > 0 && local_voxel_grid_size > 0) {
        if (local_voxel_grid_dim != voxel_grid_dim) {
            uav_voxels_resource_.Reset();
            UINT voxels_uav_size = local_voxel_grid_dim * local_voxel_grid_dim * local_voxel_grid_dim * sizeof(Voxel);
            auto& device = Game::inst()->render().device();

            HRESULT_CHECK(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(voxels_uav_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
                D3D12_RESOURCE_STATE_COMMON, nullptr,
                IID_PPV_ARGS(uav_voxels_resource_.GetAddressOf())));

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

            // voxel_grid_params_.dimension = voxel_grid_dim;
            // voxel_grid_params_.size = voxel_grid_size;
            // voxel_grid_params_.mesh_node_count = 0;
            // voxel_grid_params_buffer_.update(&voxel_grid_params_);
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
//    for (std::vector<ShaderResource<Matrix>*>& vector_matrix_srv : model_matrix_srv_) {
//        for (ShaderResource<Matrix>* matrix_srv : vector_matrix_srv) {
//            matrix_srv->destroy();
//            delete matrix_srv;
//        }
//    }
//    model_matrix_srv_.clear();
//// #ifndef NDEBUG
//    shader_voxels_draw_.destroy();
//// #endif
//    voxel_grid_params_buffer_.destroy();
//
//    shader_voxels_fill_.destroy();
//    for (std::vector<ShaderResource<MeshTreeNode>*>& mesh_tree_srv_vector : mesh_trees_) {
//        for (ShaderResource<MeshTreeNode>* mesh_tree_srv : mesh_tree_srv_vector) {
//            mesh_tree_srv->destroy();
//            delete mesh_tree_srv;
//        }
//    }
//    mesh_trees_.clear();
//
//    shader_voxels_clear_.destroy();
//    voxels_.destroy();
//
//    for (ModelTree* model_tree : model_trees_) {
//        model_tree->unload();
//        delete model_tree;
//    }
//    model_trees_.clear();
//
//    unbind_uav_.destroy();
}

