#define NOMINMAX

#include <functional>
#include <assimp/Importer.hpp>

#include "core/game.h"
#include "render/render.h"
#include "render/camera.h"
#include "model_tree.h"

const uint32_t ModelTree::Mesh::invalid_material_id = 0xFFFFFFFF;

void ModelTree::Mesh::initialize(uint32_t material_id,
    const std::vector<uint32_t>& indices,
    const std::vector<ModelTree::Mesh::Vertex>& vertices)
{
    material_id_ = material_id;
    index_count_ = static_cast<UINT>(indices.size());
    index_buffer_.initialize(indices.data(), indices.size());
    vertex_buffer_.initialize(vertices.data(), vertices.size(), sizeof(ModelTree::Mesh::Vertex));
}

void ModelTree::Mesh::destroy()
{
    material_id_ = invalid_material_id;
    index_count_ = 0;
    index_buffer_.destroy();
    vertex_buffer_.destroy();
}

void ModelTree::Mesh::draw()
{
    index_buffer_.bind();
    vertex_buffer_.bind(0U);

    auto context = Game::inst()->render().context();
    context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(index_count_, 0, 0);
}

void ModelTree::load(const std::string& file)
{
    Assimp::Importer importer;
    auto scene = importer.ReadFile(file, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
    assert(scene != nullptr);
    meshes_.reserve(scene->mNumMeshes);
    load_node(scene->mRootNode, scene);

    albedo_shader_.set_vs_shader_from_file("./resources/shaders/debug/albedo.hlsl", "VSMain");
    albedo_shader_.set_ps_shader_from_file("./resources/shaders/debug/albedo.hlsl", "PSMain");
    D3D11_INPUT_ELEMENT_DESC inputs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD_U", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD_V", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    albedo_shader_.set_input_layout(inputs, std::size(inputs));

    normal_shader_.set_vs_shader_from_file("./resources/shaders/debug/normal.hlsl", "VSMain");
    normal_shader_.set_ps_shader_from_file("./resources/shaders/debug/normal.hlsl", "PSMain");
    normal_shader_.set_input_layout(inputs, std::size(inputs));

    auto device = Game::inst()->render().device();
    CD3D11_RASTERIZER_DESC opaque_rast_desc = {};
    opaque_rast_desc.CullMode = D3D11_CULL_BACK; // do not draw back-facing triangles
    opaque_rast_desc.FillMode = D3D11_FILL_SOLID;
    opaque_rast_desc.FrontCounterClockwise = true;
    D3D11_CHECK(device->CreateRasterizerState(&opaque_rast_desc, &rasterizer_state_));

    // build tree based on extents_
    /// TODO

    // initialize GPU buffers
    dynamic_model_data_.transform = Matrix::Identity;
    dynamic_model_data_.inverse_transpose_transform = Matrix::Identity;
    if constexpr (sizeof(dynamic_model_data_) != 0) {
        dynamic_model_buffer_.initialize(&dynamic_model_data_);
    }

    albedo_shader_.attach_buffer(1U, &dynamic_model_buffer_);
    normal_shader_.attach_buffer(1U, &dynamic_model_buffer_);

    // if constexpr (sizeof(const_model_data_) != 0) {
    //     const_model_buffer_.initialize(&const_model_data_);
    // }
}

void ModelTree::unload()
{
    rasterizer_state_->Release();
    rasterizer_state_ = nullptr;

    if constexpr (sizeof(dynamic_model_data_) != 0) {
        dynamic_model_buffer_.destroy();
    }

    // if constexpr (sizeof(const_model_data_) != 0) {
    //     const_model_buffer_.destroy();
    // }

    albedo_shader_.destroy();
    normal_shader_.destroy();

    for (auto& mesh : meshes_) {
        mesh.destroy();
    }
    meshes_.clear();

}

void ModelTree::update()
{

}

void ModelTree::draw(Camera* camera)
{
    auto context = Game::inst()->render().context();
    // albedo_shader_.use();
    context->RSSetState(rasterizer_state_);

    normal_shader_.use();
    camera->bind();
    for (auto& mesh : meshes_) {
        mesh.draw();
    }
}

void ModelTree::load_node(aiNode* node, const aiScene* scene)
{
    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        load_mesh(mesh, scene);
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        load_node(node->mChildren[i], scene);
    }
}

void ModelTree::load_mesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<uint32_t> indices;
    std::vector<ModelTree::Mesh::Vertex> vertices;

    auto extendWithVertex = [this](const aiVector3D& vertex) {
        auto& [min, max] = extents_;
        min.x = std::min(min.x, vertex.x);
        min.y = std::min(min.y, vertex.y);
        min.z = std::min(min.z, vertex.z);

        max.x = std::max(max.x, vertex.x);
        max.y = std::max(max.y, vertex.y);
        max.z = std::max(max.z, vertex.z);
    };

    // fill geom data
    vertices.reserve(mesh->mNumVertices);
    for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
        vertices.push_back({ mesh->mVertices[i],
                            {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y},
                            mesh->mNormals[i] });

        extendWithVertex(mesh->mVertices[i]);
    }

    // fill indices
    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        auto& face = mesh->mFaces[i];

        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // if (mesh->mMaterialIndex >= 0) {
    //     auto mat = scene->mMaterials[mesh->mMaterialIndex];
    //     Material& material = materials_[mesh->mMaterialIndex];
    // 
    //     auto load_texture = [this, &scene](aiString& str) -> Texture* {
    //         auto texture = new Texture();
    //         auto embedded_texture = scene->GetEmbeddedTexture(str.C_Str());
    //         if (embedded_texture != nullptr) {
    //             if (embedded_texture->mHeight != 0) {
    //                 texture->initialize(embedded_texture->mWidth, embedded_texture->mHeight, DXGI_FORMAT_B8G8R8A8_UNORM, embedded_texture->pcData);
    //             }
    //         }
    //         else {
    //             auto model_path = filename_.substr(0, filename_.find_last_of('/') + 1);
    //             texture->load((model_path + str.C_Str()).c_str());
    //         }
    //         return texture;
    //     };
    // 
    //     // diffuse
    //     for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); ++i) {
    //         aiString str;
    //         mat->GetTexture(aiTextureType_DIFFUSE, i, &str);
    //         material->set_diffuse(load_texture(str));
    //     }
    // 
    //     // specular
    //     for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_SPECULAR); ++i) {
    //         aiString str;
    //         mat->GetTexture(aiTextureType_SPECULAR, i, &str);
    //         material->set_specular(load_texture(str));
    //     }
    // 
    //     // ambient
    //     for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_AMBIENT); ++i) {
    //         aiString str;
    //         mat->GetTexture(aiTextureType_AMBIENT, i, &str);
    //         material->set_ambient(load_texture(str));
    //     }
    // }

    {
        meshes_.push_back({});
        meshes_.back().initialize(mesh->mMaterialIndex, indices, vertices);
    }
}
