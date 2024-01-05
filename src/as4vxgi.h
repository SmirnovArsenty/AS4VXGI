#pragma once

#include "component/game_component.h"
#include "math/model_tree.h"

#include "resources/shaders/voxels/voxel.fx"

class AS4VXGI_Component final : public GameComponent
{
public:
    AS4VXGI_Component() = default;
    virtual ~AS4VXGI_Component() = default;

    void initialize() override;
    void draw() override;
    void imgui() override;
    void reload() override;
    void update() override;
    void destroy_resources() override;
private:
    std::vector<ModelTree> model_trees_;

    // voxels storage
    UnorderedAccessBuffer<Voxel> voxels_;

    // stage1 - clear storage
    ComputeShader shader_voxels_clear_;

    // stage2 - fill storage with vertices params
    ComputeShader shader_voxels_fill_;
    std::vector<ShaderResource<MeshTreeNode>> mesh_trees_;
    std::vector<ID3D11ShaderResourceView*> index_buffers_srv_;
    std::vector<ID3D11ShaderResourceView*> vertex_buffers_srv_;
    VoxelGrid voxel_grid_params_;
    DynamicBuffer<decltype(voxel_grid_params_)> voxel_grid_params_buffer_;
#ifndef NDEBUG
    GraphicsShader shader_voxels_draw_;
#endif
};
