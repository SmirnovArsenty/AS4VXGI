#include "core/game.h"
#include "render/render.h"
#include "render/camera.h"
#include "render/annotation.h"

#include "as4vxgi.h"

constexpr int32_t voxel_grid_dim = 10;
constexpr float voxel_grid_size = 100;
inline int align(int value, int alignment)
{
    return (value + (alignment - 1)) & ~(alignment - 1);
}

void AS4VXGI_Component::initialize()
{
    model_trees_.push_back({});
    ModelTree& model = model_trees_.back();
    //model.load("./resources/models/suzanne.fbx");
    //model.load("./resources/models/terrain.fbx");
    //model.load("./resources/models/sponza/source/sponza.fbx");

    // sync
    complete_buffer_.initialize(nullptr, 1, D3D11_BUFFER_UAV_FLAG_APPEND);
    complete_buffer_cpu_.initialize();
    shader_complete_.set_compute_shader_from_file("./resources/shaders/voxels/complete.hlsl", "CSMain");
    shader_complete_reset_.set_compute_shader_from_file("./resources/shaders/voxels/complete_reset.hlsl", "CSMain");

    shader_complete_.attach_uav(0, &complete_buffer_);
    shader_complete_reset_.attach_uav(0, &complete_buffer_);

    // voxels storage
    voxels_.initialize(nullptr, voxel_grid_dim * voxel_grid_dim * voxel_grid_dim);
    voxels_.set_name("voxel storage");

    // clear voxels
    shader_voxels_clear_.set_compute_shader_from_file("./resources/shaders/voxels/clear.hlsl", "CSMain");
    shader_voxels_clear_.set_name("voxels_clear");
    shader_voxels_clear_.attach_uav(0, &voxels_);

    // fill voxels
    for (ModelTree& mode_tree : model_trees_) {
        for (ID3D11ShaderResourceView* srv :model.get_index_buffers_srv())
            index_buffers_srv_.push_back(srv);
        for (ID3D11ShaderResourceView* srv : model.get_vertex_buffers_srv())
            vertex_buffers_srv_.push_back(srv);
        for (std::vector<MeshTreeNode> mesh_tree : model.get_meshes_trees()) {
            mesh_trees_.push_back({});
            mesh_trees_.back().initialize(mesh_tree.data(), mesh_tree.size());
        }
    }
    voxel_grid_params_.dimension = voxel_grid_dim;
    voxel_grid_params_.size = voxel_grid_size;
    voxel_grid_params_.mesh_node_count = 0;
    voxel_grid_params_buffer_.initialize(&voxel_grid_params_);
    shader_voxels_fill_.set_compute_shader_from_file("./resources/shaders/voxels/fill.hlsl", "CSMain");
    shader_voxels_fill_.set_name("voxels_fill");
#ifndef NDEBUG
    shader_voxels_draw_.set_vs_shader_from_file("./resources/shaders/voxels/draw.hlsl", "VSMain");
    shader_voxels_draw_.set_gs_shader_from_file("./resources/shaders/voxels/draw.hlsl", "GSMain");
    shader_voxels_draw_.set_ps_shader_from_file("./resources/shaders/voxels/draw.hlsl", "PSMain");
    shader_voxels_draw_.set_name("voxels_draw_debug");
#endif
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
            {
                /*Annotation rence_reset("fence reset");
                shader_complete_reset_.use();
                context->Dispatch(1, 1, 1);*/
            }

            // stage 2
            Annotation fill("fill voxel params");
            shader_voxels_fill_.use();
            for (int i = 0; i < index_buffers_srv_.size(); ++i) {
                voxel_grid_params_.mesh_node_count = static_cast<int32_t>(mesh_trees_[i].size());
                voxel_grid_params_buffer_.update(&voxel_grid_params_);
                context->CSSetConstantBuffers(1, 1, &voxel_grid_params_buffer_.getBuffer());

                context->CSSetShaderResources(0, 1, &mesh_trees_[i].getSRV());
                context->CSSetShaderResources(1, 1, &index_buffers_srv_[i]);
                context->CSSetShaderResources(2, 1, &vertex_buffers_srv_[i]);

                context->Dispatch(voxel_grid_dim, voxel_grid_dim, voxel_grid_dim);
            }

            // fence
            {
                Annotation fence("fence");
                shader_complete_.use();
                context->Dispatch(1, 1, 1);

                context->CopyStructureCount(complete_buffer_cpu_.getBuffer(), 0, complete_buffer_.getUAV());
                int32_t complete = complete_buffer_cpu_.data();
                while (complete == 0) {
                    context->CopyStructureCount(complete_buffer_cpu_.getBuffer(), 0, complete_buffer_.getUAV());
                    complete = complete_buffer_cpu_.data();
                }

                shader_complete_reset_.use();
                context->Dispatch(1, 1, 1);
            }

#ifndef NDEBUG
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
#endif
        }
    }

    for (auto& model_tree : model_trees_) {
        model_tree.draw(Game::inst()->render().camera());
    }
}

void AS4VXGI_Component::imgui()
{

}

void AS4VXGI_Component::reload()
{

}

void AS4VXGI_Component::update()
{

}

void AS4VXGI_Component::destroy_resources()
{
    shader_voxels_draw_.destroy();
    voxel_grid_params_buffer_.destroy();

    shader_voxels_fill_.destroy();
    for (auto& srv : mesh_trees_) {
        srv.destroy();
    }
    mesh_trees_.clear();

    shader_voxels_clear_.destroy();
    voxels_.destroy();

    for (auto& model_tree : model_trees_) {
        model_tree.unload();
    }
    model_trees_.clear();
}
