#pragma once

#include <string>
#include <unordered_map>

#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "render/resource/buffer.h"
#include "render/resource/texture.h"
#include "render/resource/sampler.h"
#include "render/resource/material.h"
#include "render/resource/shader.h"

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

    void draw(Camera* camera);

    std::vector<ID3D11ShaderResourceView*> get_index_buffers_srv();
    std::vector<ID3D11ShaderResourceView*> get_vertex_buffers_srv();
    Matrix get_transform() const { return dynamic_model_data_.transform; }

    std::vector<std::vector<MeshTreeNode>> get_meshes_trees();
private:
    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh() = default;

        void initialize(Material& material,
                        const std::vector<uint32_t>& indices,
                        const std::vector<Vertex>& vertices,
                        float min[3], float max[3]);

        void destroy();

        void draw();
#ifndef NDEBUG
        void debug_draw();
#endif

        ID3D11ShaderResourceView* get_index_buffer_srv() { return index_buffer_resource_view_.getSRV(); }
        ID3D11ShaderResourceView* get_vertex_buffer_srv() { return vertex_buffer_resource_view_.getSRV(); }

        std::vector<MeshTreeNode> get_mesh_tree() const;
    private:
        Material material_;

        // mesh extents
        float min_[3];
        float max_[3];

        std::vector<uint32_t> indices_;
        std::vector<Vertex> vertices_;

        UINT index_count_{ 0 };
        IndexBuffer index_buffer_;
        VertexBuffer vertex_buffer_;
        ShaderResource<uint32_t> index_buffer_resource_view_;
        ShaderResource<Vertex> vertex_buffer_resource_view_;

        std::unordered_map<int32_t, MeshTreeNode> mesh_tree_;

        void split_vertices(float min[3], float max[3], int32_t start, int32_t count, int32_t current_mesh_node_index);

#ifndef NDEBUG
        ShaderResource<Matrix> box_transformations_;
        GraphicsShader box_shader_;
        IndexBuffer box_index_buffer_;
        VertexBuffer box_vertex_buffer_;
#endif
    };

    void load_node(aiNode* node, const aiScene* scene);
    void load_mesh(aiMesh* mesh, const aiScene* scene);

    std::vector<Mesh*> meshes_;

    // struct {
    //     uint32_t dummy[4];
    // } const_model_data_;
    // ConstBuffer<decltype(const_model_data_)> const_model_buffer_;

    struct {
        Matrix transform;
        Matrix inverse_transpose_transform;
    } dynamic_model_data_;
    DynamicBuffer<decltype(dynamic_model_data_)> dynamic_model_buffer_;

    // renderstate
    ID3D11RasterizerState* rasterizer_state_{ nullptr };

    // debug shaders
    GraphicsShader albedo_shader_;
    GraphicsShader normal_shader_;
};
