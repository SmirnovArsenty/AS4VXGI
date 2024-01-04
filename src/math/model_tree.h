#pragma once

#include <string>

#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "render/resource/buffer.h"
#include "render/resource/texture.h"
#include "render/resource/shader.h"

#include "SimpleMath.h"
using namespace DirectX::SimpleMath;

class Camera;

class ModelTree
{
public:
    ModelTree() = default;
    ~ModelTree() = default;

    // load model to tree
    // allocate resources
    void load(const std::string& path);

    // destroy resources
    void unload();

    void update();

    void draw(Camera* camera);
private:
    class Material
    {
    public:
        Material() = default;
        ~Material() = default;
    private:
        Texture diffuse_;
    };

    class Mesh
    {
    public:
#pragma pack(push, 1)
        struct MeshTreeNode
        {
            float min[3]{ FLT_MAX };
            int32_t start_index;
            float max[3]{ FLT_MIN };
            int32_t count;
        };

        struct Vertex
        {
            aiVector3D position;
            aiVector2D uv;
            aiVector3D normal;
        };
#pragma pack(pop)

        Mesh() = default;
        ~Mesh() = default;

        void initialize(uint32_t material_id,
                        const std::vector<uint32_t>& indices,
                        const std::vector<Vertex>& vertices,
                        float min[3], float max[3]);

        void destroy();

        void draw();
#ifndef NDEBUG
        void debug_draw();
#endif
    private:
        static constexpr uint32_t invalid_material_id = 0xFFFFFFFF;
        uint32_t material_id_ = invalid_material_id;

        // mesh extents
        float min_[3];
        float max_[3];

        std::vector<uint32_t> indices_;
        std::vector<Vertex> vertices_;

        UINT index_count_{ 0 };
        IndexBuffer index_buffer_;
        VertexBuffer vertex_buffer_;

        std::vector<MeshTreeNode> mesh_tree_;

        void split_vertices(float min[3], float max[3], int32_t start, int32_t count);

#ifndef NDEBUG
        ShaderResource<Matrix> box_transformations_;
        static GraphicsShader* box_shader_;
        static IndexBuffer* box_index_buffer_;
        static VertexBuffer* box_vertex_buffer_;
#endif
    };

    void load_node(aiNode* node, const aiScene* scene);
    void load_mesh(aiMesh* mesh, const aiScene* scene);

    std::vector<Mesh> meshes_;

    std::unordered_map<uint32_t, Material> materials_;

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
