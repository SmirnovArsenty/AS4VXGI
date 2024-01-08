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
    std::vector<ModelTree*> model_trees_;

    // sync mechanism
    struct {
        int32_t data[4]{};
    } complete_data_;
    UnorderedAccessBuffer<decltype(complete_data_)> complete_buffer_;
    ReadbackBuffer<uint32_t> complete_buffer_cpu_;
    ComputeShader shader_complete_;
    // ComputeShader shader_complete_reset_;
    void fence();

    // voxels storage
    UnorderedAccessBuffer<Voxel> voxels_;

    /////
    // stage1 - clear storage
    /////

    ComputeShader shader_voxels_clear_;

    /////
    // stage2 - fill storage with vertices params
    /////

    std::vector<std::vector<ShaderResource<MeshTreeNode>*>> mesh_trees_;
    ComputeShader shader_voxels_fill_;
    std::vector<std::vector<ID3D11ShaderResourceView*>> index_buffers_srv_;
    std::vector<std::vector<ID3D11ShaderResourceView*>> vertex_buffers_srv_;
    std::vector<std::vector<ShaderResource<Matrix>*>> model_matrix_srv_;
    VoxelGrid voxel_grid_params_;
    DynamicBuffer<decltype(voxel_grid_params_)> voxel_grid_params_buffer_;
// #ifndef NDEBUG
    GraphicsShader shader_voxels_draw_;
// #endif
};
