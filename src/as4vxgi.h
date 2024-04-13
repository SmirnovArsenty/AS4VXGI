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

    VOXEL_DATA_BIND voxel_data_;
    ConstBuffer<VOXEL_DATA_BIND> voxel_data_cb_;

    ComPtr<ID3D12Resource> uav_voxels_resource_;
    UINT uav_voxels_resource_index_;
    D3D12_CPU_DESCRIPTOR_HANDLE uav_voxels_;
    D3D12_GPU_DESCRIPTOR_HANDLE uav_voxels_gpu_;

    /////
    // stage1 - clear storage
    ComputePipeline* stage_1_pipeline_ = nullptr;
    /////

    /////
    // stage2 - fill storage with vertices params
    ComputePipeline* stage_2_pipeline_ = nullptr;
    /////

    std::vector<std::vector<ShaderResource<MeshTreeNode>*>> mesh_trees_srv_;
    std::vector<std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>> index_buffers_srv_;
    std::vector<std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>> vertex_buffers_srv_;
    std::vector<std::vector<ShaderResource<Matrix>*>> model_matrix_srv_;
// #ifndef NDEBUG
    ComPtr<ID3D12Fence> fence_ = nullptr;
    uint64_t fence_value_ = 0;
    GraphicsPipeline* stage_visualize_pipeline_ = nullptr;
// #endif
};
