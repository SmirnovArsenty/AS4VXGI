#pragma once

#include "render/common.h"
#include "component/game_component.h"
#include "math/model_tree.h"
#include "render/resource/pipeline.h"

#include "resources/shaders/voxels/voxel.fx"

#include <mutex>

class AS4VXGI_Component final : public GameComponent
{
public:
    AS4VXGI_Component() = default;
    ~AS4VXGI_Component() override = default;

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

    ComputePipeline voxels_fill_;

    ComPtr<ID3D12Resource> uav_voxels_resource_{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE uav_voxels_cpu_;
    UINT uav_voxels_resource_index_;
    UINT uav_voxels_resource_index_cpu_;
    D3D12_CPU_DESCRIPTOR_HANDLE uav_voxels_;
    D3D12_GPU_DESCRIPTOR_HANDLE uav_voxels_gpu_;

    std::vector<std::vector<ShaderResource<MeshTreeNode>*>> mesh_trees_srv_;
    std::vector<std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>> index_buffers_srv_;
    std::vector<std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>> vertex_buffers_srv_;
    std::vector<std::vector<ShaderResource<Matrix>*>> model_matrix_srv_;
// #ifndef NDEBUG
    GraphicsPipeline stage_visualize_pipeline_;
// #endif
};
