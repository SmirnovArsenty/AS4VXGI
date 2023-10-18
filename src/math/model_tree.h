#pragma once

#include <string>

#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "render/resource/buffer.h"
#include "render/resource/texture.h"
#include "render/resource/shader.h"

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
        struct Vertex
        {
            aiVector3D position;
            aiVector2D texcoord;
            aiVector3D normal;
        };
#pragma pack(pop)

        Mesh() = default;
        ~Mesh() = default;

        void initialize(uint32_t material_id,
                        const std::vector<uint32_t>& indices,
                        const std::vector<Vertex>& vertices);

        void destroy();

        void draw();
    private:
        static const uint32_t invalid_material_id;
        uint32_t material_id_ = invalid_material_id;

        UINT index_count_{ 0 };
        IndexBuffer index_buffer_;
        VertexBuffer vertex_buffer_;
    };

    void load_node(aiNode* node, const aiScene* scene);
    void load_mesh(aiMesh* mesh, const aiScene* scene);

    std::pair<aiVector3D, aiVector3D> extents_{ aiVector3D(FLT_MAX), aiVector3D(FLT_MIN) };

    std::vector<Mesh> meshes_;

    std::unordered_map<uint32_t, Material> materials_;
};
