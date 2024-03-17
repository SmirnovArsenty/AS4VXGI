#pragma once

#include "render/common.h"
#include "component/game_component.h"
#include "math/model_tree.h"
#include "render/resource/pipeline.h"

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

    ConstBuffer<COMMON_BIND> common_cb_;
    COMMON_BIND common_;

    ComPtr<ID3D12Resource> uav_voxels_resource_;
    UINT uav_voxels_resource_index_;
    D3D12_CPU_DESCRIPTOR_HANDLE uav_voxels_;

    // voxels storage
    // UnorderedAccessBuffer<Voxel> voxels_;

    /////
    // stage1 - clear storage
    ComputePipeline* stage_1_pipeline_ = nullptr;
    /////

    // ComputeShader shader_voxels_clear_;

    /////
    // stage2 - fill storage with vertices params
    ComputePipeline* stage_2_pipeline_ = nullptr;
    /////

    // std::vector<std::vector<ShaderResource<MeshTreeNode>*>> mesh_trees_;
    // ComputeShader shader_voxels_fill_;
    // std::vector<std::vector<ID3D11ShaderResourceView*>> index_buffers_srv_;
    // std::vector<std::vector<ID3D11ShaderResourceView*>> vertex_buffers_srv_;
    // std::vector<std::vector<ShaderResource<Matrix>*>> model_matrix_srv_;
    // VoxelGrid voxel_grid_params_;
    // DynamicBuffer<decltype(voxel_grid_params_)> voxel_grid_params_buffer_;
// #ifndef NDEBUG
    GraphicsPipeline* stage_visualize_pipeline_ = nullptr;
    // GraphicsShader shader_voxels_draw_;
// #endif
};
