#include <imgui/imgui.h>

#include "core/game.h"
#include "render/render.h"
#include "render/camera.h"
#include "render/annotation.h"

#include "as4vxgi.h"

int32_t voxel_grid_dim = 50;
float voxel_grid_size = 1000;

inline int align(int value, int alignment)
{
    return (value + (alignment - 1)) & ~(alignment - 1);
}

void AS4VXGI_Component::initialize()
{
    Game::inst()->render().camera()->set_camera(Vector3(500, 0, 0), Vector3(-1, 0, 0));

    model_trees_.push_back(new ModelTree{});
    model_trees_.back()->load("./resources/models/suzanne.fbx");//, Vector3(0, 0, 150));
    //model_trees_.push_back(new ModelTree{});
    //model_trees_.back()->load("./resources/models/suzanne.fbx", Vector3(0, 0, -150));
    //model.load("./resources/models/terrain.fbx");
    //model.load("./resources/models/sponza/source/sponza.fbx");

    // sync
    complete_buffer_.initialize(nullptr, 1, D3D11_BUFFER_UAV_FLAG_APPEND);
    complete_buffer_cpu_.initialize();
    shader_complete_.set_compute_shader_from_file("./resources/shaders/voxels/complete.hlsl", "CSMain");

    // voxels storage
    voxels_.initialize(nullptr, voxel_grid_dim * voxel_grid_dim * voxel_grid_dim);
    voxels_.set_name("voxel storage");

    // clear voxels
    shader_voxels_clear_.set_compute_shader_from_file("./resources/shaders/voxels/clear.hlsl", "CSMain");
    shader_voxels_clear_.set_name("voxels_clear");
    shader_voxels_clear_.attach_uav(0, &voxels_);

    // fill voxels
    for (int32_t i = 0; i < model_trees_.size(); ++i) {
        ModelTree* model_tree = model_trees_[i];
        Matrix transform = model_tree->get_transform();

        index_buffers_srv_.push_back({});
        for (ID3D11ShaderResourceView* srv : model_tree->get_index_buffers_srv()) {
            index_buffers_srv_[i].push_back(srv);
        }
        vertex_buffers_srv_.push_back({});
        for (ID3D11ShaderResourceView* srv : model_tree->get_vertex_buffers_srv()) {
            vertex_buffers_srv_[i].push_back(srv);
        }
        mesh_trees_.push_back({});
        model_matrix_srv_.push_back({});
        for (std::vector<MeshTreeNode> mesh_tree : model_tree->get_meshes_trees()) {
            mesh_trees_[i].push_back(new ShaderResource<MeshTreeNode>());
            mesh_trees_[i].back()->initialize(mesh_tree.data(), mesh_tree.size());

            model_matrix_srv_[i].push_back({nullptr});
            model_matrix_srv_[i].back() = new ShaderResource<Matrix>();
            model_matrix_srv_[i].back()->initialize(&transform, 1);
        }
    }
    voxel_grid_params_.dimension = voxel_grid_dim;
    voxel_grid_params_.size = voxel_grid_size;
    voxel_grid_params_.mesh_node_count = 0;
    voxel_grid_params_buffer_.initialize(&voxel_grid_params_);
    shader_voxels_fill_.set_compute_shader_from_file("./resources/shaders/voxels/fill.hlsl", "CSMain");
    shader_voxels_fill_.set_name("voxels_fill");
// #ifndef NDEBUG
    shader_voxels_draw_.set_vs_shader_from_file("./resources/shaders/voxels/draw.hlsl", "VSMain");
    shader_voxels_draw_.set_gs_shader_from_file("./resources/shaders/voxels/draw.hlsl", "GSMain");
    shader_voxels_draw_.set_ps_shader_from_file("./resources/shaders/voxels/draw.hlsl", "PSMain");
    shader_voxels_draw_.set_name("voxels_draw_debug");
// #endif
}

void AS4VXGI_Component::draw()
{
    auto context = Game::inst()->render().context();
    Game::inst()->render().camera()->bind();

    {
        Annotation voxelization("voxelization");
        {
            // stage 1
            Annotation clear("clear");
            shader_voxels_clear_.use();
            context->Dispatch(align(voxel_grid_dim * voxel_grid_dim * voxel_grid_dim, 256) / 256, 1, 1);
        }

        {
            // stage 2
            Annotation fill("fill voxel params");
            shader_voxels_fill_.use();
            for (int i = 0; i < mesh_trees_.size(); ++i) {
                for (int32_t j = 0; j < mesh_trees_[i].size(); ++j) {

                    voxel_grid_params_.mesh_node_count = static_cast<int32_t>(mesh_trees_[i][j]->size());
                    voxel_grid_params_buffer_.update(&voxel_grid_params_);
                    context->CSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());

                    context->CSSetShaderResources(0, 1, &mesh_trees_[i][j]->getSRV());
                    context->CSSetShaderResources(1, 1, &index_buffers_srv_[i][j]);
                    context->CSSetShaderResources(2, 1, &vertex_buffers_srv_[i][j]);
                    context->CSSetShaderResources(3, 1, &model_matrix_srv_[i][j]->getSRV());

                    context->Dispatch(align(voxel_grid_dim, 4) / 4,
                                      align(voxel_grid_dim, 4) / 4,
                                      align(voxel_grid_dim, 4) / 4);
                }
            }
            fence();

// #ifndef NDEBUG
            // visualize voxel grid
            {
                Annotation debug("voxels debug");
                shader_voxels_draw_.use();
                context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

                context->VSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());
                context->HSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());
                context->DSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());
                context->GSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());
                context->PSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());

                context->VSSetShaderResources(0, 1, &voxels_.getSRV());

                context->DrawInstanced(1, voxel_grid_dim * voxel_grid_dim * voxel_grid_dim, 0, 0);
            }
// #endif
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
            voxels_.destroy();
            voxels_.initialize(nullptr, local_voxel_grid_dim * local_voxel_grid_dim * local_voxel_grid_dim);
            voxels_.set_name("voxel storage");
        }

        if (local_voxel_grid_size != voxel_grid_size || local_voxel_grid_dim != voxel_grid_dim) {
            voxel_grid_size = local_voxel_grid_size;
            voxel_grid_dim = local_voxel_grid_dim;

            voxel_grid_params_.dimension = voxel_grid_dim;
            voxel_grid_params_.size = voxel_grid_size;
            voxel_grid_params_.mesh_node_count = 0;
            voxel_grid_params_buffer_.update(&voxel_grid_params_);
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
    shader_complete_.destroy();
    complete_buffer_.destroy();
    complete_buffer_cpu_.destroy();

    for (std::vector<ShaderResource<Matrix>*>& vector_matrix_srv : model_matrix_srv_) {
        for (ShaderResource<Matrix>* matrix_srv : vector_matrix_srv) {
            matrix_srv->destroy();
            delete matrix_srv;
        }
    }
    model_matrix_srv_.clear();
// #ifndef NDEBUG
    shader_voxels_draw_.destroy();
// #endif
    voxel_grid_params_buffer_.destroy();

    shader_voxels_fill_.destroy();
    for (std::vector<ShaderResource<MeshTreeNode>*>& mesh_tree_srv_vector : mesh_trees_) {
        for (ShaderResource<MeshTreeNode>* mesh_tree_srv : mesh_tree_srv_vector) {
            mesh_tree_srv->destroy();
            delete mesh_tree_srv;
        }
    }
    mesh_trees_.clear();

    shader_voxels_clear_.destroy();
    voxels_.destroy();

    for (ModelTree* model_tree : model_trees_) {
        model_tree->unload();
        delete model_tree;
    }
    model_trees_.clear();
}

void AS4VXGI_Component::fence()
{
    Annotation fence("fence");

    auto context = Game::inst()->render().context();
    shader_complete_.use();
    UINT initial = 0;
    context->CSSetUnorderedAccessViews(0, 1, &complete_buffer_.getUAV(), &initial);
    context->Dispatch(1, 1, 1);

    context->CopyStructureCount(complete_buffer_cpu_.getBuffer(), 0, complete_buffer_.getUAV());
    int32_t complete = complete_buffer_cpu_.data();
    while (complete == 0) {
        context->CopyStructureCount(complete_buffer_cpu_.getBuffer(), 0, complete_buffer_.getUAV());
        complete = complete_buffer_cpu_.data();
    }
}
