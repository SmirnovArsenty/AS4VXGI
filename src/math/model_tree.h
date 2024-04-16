#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "render/common.h"
#include "render/resource/buffer.hpp"
#include "render/resource/texture.h"
#include "render/resource/pipeline.h"

#include "SimpleMath.h"
using namespace DirectX::SimpleMath;

#include "resources/shaders/voxels/voxel.fx"

class Camera;

class ModelTree
{
public:
    ModelTree() = default;
    ~ModelTree() = default;

    // load model to tree
    // allocate resources
    void load(const std::string& path, Vector3 position = Vector3(), Quaternion rotation = Quaternion(), Vector3 scale = Vector3(1, 1, 1));

    // destroy resources
    void unload();

    void update();

    void draw(Camera* camera, ID3D12GraphicsCommandList* cmd_list);

    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> get_index_buffers_srv();
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> get_vertex_buffers_srv();
    std::vector<std::vector<uint32_t>> get_meshes_indices();
    std::vector<std::vector<Vertex>> get_meshes_vertices();
    Matrix get_transform() const { return model_data_.transform; }

    std::vector<std::vector<MeshTreeNode>> get_meshes_trees();
private:
    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh() = default;

        void initialize(//Material& material,
                        const std::vector<uint32_t>& indices,
                        const std::vector<Vertex>& vertices,
                        float min[3], float max[3]);

        void destroy();

#ifndef NDEBUG
        const D3D12_INDEX_BUFFER_VIEW& get_box_index_buffer_view() { return box_index_buffer_.view(); }
        const D3D12_VERTEX_BUFFER_VIEW& get_box_vertex_buffer_view() { return box_vertex_buffer_.view(); }
        D3D12_GPU_DESCRIPTOR_HANDLE box_transformation_srv_gpu_handle();
#endif

        const D3D12_INDEX_BUFFER_VIEW& get_index_buffer_view() { return index_buffer_.view(); }
        D3D12_GPU_DESCRIPTOR_HANDLE& get_index_buffer_srv() { return index_buffer_srv_gpu_; }
        const D3D12_VERTEX_BUFFER_VIEW& get_vertex_buffer_view() { return vertex_buffer_.view(); }
        D3D12_GPU_DESCRIPTOR_HANDLE& get_vertex_buffer_srv() { return vertex_buffer_srv_gpu_; }

        const std::vector<uint32_t>& get_indices() const;
        const std::vector<Vertex>& get_vertices() const;

        std::vector<MeshTreeNode> get_mesh_tree() const;
    private:
        // Material material_;

        // mesh extents
        float min_[3];
        float max_[3];

        std::vector<uint32_t> indices_;
        std::vector<Vertex> vertices_;

        UINT index_count_{ 0 };

        IndexBuffer index_buffer_;
        VertexBuffer<Vertex> vertex_buffer_;

        D3D12_CPU_DESCRIPTOR_HANDLE index_buffer_srv_;
        D3D12_GPU_DESCRIPTOR_HANDLE index_buffer_srv_gpu_;
        UINT index_buffer_srv_resource_index_;
        D3D12_CPU_DESCRIPTOR_HANDLE vertex_buffer_srv_;
        D3D12_GPU_DESCRIPTOR_HANDLE vertex_buffer_srv_gpu_;
        UINT vertex_buffer_srv_resource_index_;

        std::unordered_map<int32_t, MeshTreeNode> mesh_tree_;

        void split_vertices(float min[3], float max[3], int32_t start, int32_t count, int32_t current_mesh_node_index);

#ifndef NDEBUG
        ComPtr<ID3D12Resource> box_transformations_;
        void* box_transformations_mapped_ptr_ = nullptr;
        UINT box_transformation_resource_index_;
        D3D12_CPU_DESCRIPTOR_HANDLE box_transformations_srv_;
        D3D12_GPU_DESCRIPTOR_HANDLE box_transformations_srv_gpu_;

        IndexBuffer box_index_buffer_;
        VertexBuffer<Vector4> box_vertex_buffer_;
#endif
    };

    void load_node(aiNode* node, const aiScene* scene);
    void load_mesh(aiMesh* mesh, const aiScene* scene);

    std::vector<Mesh*> meshes_;

    // struct {
    //     uint32_t dummy[4];
    // } const_model_data_;
    // ConstBuffer<decltype(const_model_data_)> const_model_buffer_;

    MODEL_DATA_BIND model_data_;
    ConstBuffer<decltype(model_data_)> model_cb_;
    // DynamicBuffer<decltype(dynamic_model_data_)> dynamic_model_buffer_;

    // renderstate
    // ID3D11RasterizerState* rasterizer_state_{ nullptr };

    // debug shaders
    // GraphicsShader albedo_shader_;
    // GraphicsShader normal_shader_;

    GraphicsPipeline graphics_pipeline_;

#ifndef NDEBUG
    GraphicsPipeline box_visualize_pipeline_;
#endif
};
