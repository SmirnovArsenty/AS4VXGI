#define NOMINMAX

#include <functional>
#include <assimp/Importer.hpp>

#include "core/game.h"
#include "render/render.h"
#include "render/camera.h"
#include "model_tree.h"

GraphicsShader* ModelTree::Mesh::box_shader_ = nullptr;
IndexBuffer* ModelTree::Mesh::box_index_buffer_ = nullptr;
VertexBuffer* ModelTree::Mesh::box_vertex_buffer_ = nullptr;

void ModelTree::Mesh::initialize(uint32_t material_id,
    const std::vector<uint32_t>& indices,
    const std::vector<ModelTree::Mesh::Vertex>& vertices,
    float min[3], float max[3])
{
    material_id_ = material_id;
    for (int32_t i = 0; i < _countof(min_); ++i) {
        min_[i] = min[i];
        max_[i] = max[i];
    }
    indices_ = indices;
    vertices_ = vertices;
    index_count_ = static_cast<UINT>(indices.size());

    split_vertices(min_, max_, 0, index_count_);

    index_buffer_.initialize(indices.data(), indices.size());
    vertex_buffer_.initialize(vertices.data(), vertices.size(), sizeof(ModelTree::Mesh::Vertex));

#ifndef NDEBUG
    if (box_shader_ == nullptr) {
        box_shader_ = new GraphicsShader();
        box_shader_->set_vs_shader_from_file("./resources/shaders/debug/box.hlsl", "VSMain");
        box_shader_->set_ps_shader_from_file("./resources/shaders/debug/box.hlsl", "PSMain");
        D3D11_INPUT_ELEMENT_DESC inputs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        box_shader_->set_input_layout(inputs, std::size(inputs));

        std::vector<uint32_t> box_indices;
        std::vector<Vector4> box_vertices;
        box_vertices.push_back({1, 1, 1, 1}); // 0
        box_vertices.push_back({1, 1, -1, 1}); // 1
        box_vertices.push_back({1, -1, 1, 1}); // 2
        box_vertices.push_back({-1, 1, 1, 1}); // 3
        box_vertices.push_back({1, -1, -1, 1}); // 4
        box_vertices.push_back({-1, 1, -1, 1}); // 5
        box_vertices.push_back({-1, -1, 1, 1}); // 6
        box_vertices.push_back({-1, -1, -1, 1}); // 7

        box_indices.push_back(0);
        box_indices.push_back(1);
        box_indices.push_back(0);
        box_indices.push_back(2);
        box_indices.push_back(0);
        box_indices.push_back(3);

        box_indices.push_back(1);
        box_indices.push_back(4);
        box_indices.push_back(1);
        box_indices.push_back(5);

        box_indices.push_back(2);
        box_indices.push_back(4);
        box_indices.push_back(2);
        box_indices.push_back(6);

        box_indices.push_back(3);
        box_indices.push_back(5);
        box_indices.push_back(3);
        box_indices.push_back(6);

        box_indices.push_back(4);
        box_indices.push_back(7);

        box_indices.push_back(5);
        box_indices.push_back(7);

        box_indices.push_back(6);
        box_indices.push_back(7);

        assert(box_indices.size() == 24); // 12 edges

        box_index_buffer_ = new IndexBuffer();
        box_index_buffer_->initialize(box_indices.data(), box_indices.size());
        box_vertex_buffer_ = new VertexBuffer();
        box_vertex_buffer_->initialize(box_vertices.data(), box_vertices.size(), sizeof(ModelTree::Mesh::Vertex));

        box_shader_->attach_index_buffer(box_index_buffer_);
        box_shader_->attach_vertex_buffer(box_vertex_buffer_);
    }

    std::vector<Matrix> box_transformations;
    for (int32_t i = 0; i < mesh_tree_.size(); ++i) {
        aiVector3D position;
        aiVector3D scale;

        for (int32_t j = 0; j < 3; ++j) {
            position[j] = (mesh_tree_[i].max[j] + mesh_tree_[i].min[j]) / 2;
            scale[j] = mesh_tree_[i].max[j] - mesh_tree_[i].min[j];
        }

        box_transformations.push_back(Matrix::CreateTranslation(position.x, position.y, position.z) * Matrix::CreateScale(scale.x, scale.y, scale.z));
    }
    box_transformations_.initialize(box_transformations.data(), (int32_t)std::size(box_transformations));
#endif
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

#ifndef NDEBUG
void ModelTree::Mesh::debug_draw()
{
    box_shader_->use();

    auto context = Game::inst()->render().context();
    context->VSSetShaderResources(0, 1, &box_transformations_.getSRV());
    context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    context->DrawIndexedInstanced(24, (int32_t)std::size(mesh_tree_), 0, 0, 0);
}
#endif

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

#ifndef NDEBUG
    for (auto& mesh : meshes_) {
        mesh.debug_draw();
    }
#endif
}

void ModelTree::Mesh::split_vertices(float min[3], float max[3], int32_t start, int32_t count)
{
    constexpr float smallest_length = 0.5f;

    const float length[3] = { max[0] - min[0], max[1] - min[1], max[2] - min[2] };
    const float mean[3] = { (max[0] + min[0]) / 2, (max[1] + min[1]) / 2, (max[2] + min[2]) / 2 };
    int32_t split_index = -1;
    if (length[0] > length[1] && length[0] > length[2] && length[0] > smallest_length) {
        split_index = 0;
    } else if (length[1] > length[0] && length[1] > length[2] && length[1] > smallest_length) {
        split_index = 1;
    } else if (length[2] > length[0] && length[2] > length[1] && length[2] > smallest_length) {
        split_index = 2;
    }

    // radix sort

    std::vector<int32_t> indices[3]; // 0 - parent, 1 - less child, 2 - greater child
    for (int i = start; i < start + count; i += 3) {
        float triangle[3][3] = {
            { vertices_[indices_[i + 0]].position.x, vertices_[indices_[i + 0]].position.y, vertices_[indices_[i + 0]].position.z },
            { vertices_[indices_[i + 1]].position.x, vertices_[indices_[i + 1]].position.y, vertices_[indices_[i + 1]].position.z },
            { vertices_[indices_[i + 2]].position.x, vertices_[indices_[i + 2]].position.y, vertices_[indices_[i + 2]].position.z }
        };
        if (split_index != -1 && triangle[0][split_index] < mean[split_index] && triangle[1][split_index] < mean[split_index] && triangle[2][split_index] < mean[split_index]) {
            indices[1].push_back(indices_[i + 0]);
            indices[1].push_back(indices_[i + 1]);
            indices[1].push_back(indices_[i + 2]);
        } else if (split_index != -1 && triangle[0][split_index] > mean[split_index] && triangle[1][split_index] > mean[split_index] && triangle[2][split_index] > mean[split_index]) {
            indices[2].push_back(indices_[i + 0]);
            indices[2].push_back(indices_[i + 1]);
            indices[2].push_back(indices_[i + 2]);
        } else {
            indices[0].push_back(indices_[i + 0]);
            indices[0].push_back(indices_[i + 1]);
            indices[0].push_back(indices_[i + 2]);
        }
    }

    MeshTreeNode node;
    memcpy(node.min, min, sizeof(float) * 3);
    memcpy(node.max, max, sizeof(float) * 3);
    node.start_index = start;
    node.count = (int32_t)indices[0].size();
    mesh_tree_.push_back(node);

    {
        int32_t offset = start;
        for (int32_t i = 0; i < 3; ++i) {
            for (int32_t j = 0; j < indices[i].size(); ++j) {
                indices_[j + offset] = indices[i][j];
            }
            offset += (int32_t)indices[i].size();
        }
    }

    if (split_index != -1) {
        int32_t offset = start + (int32_t)indices[0].size();
        for (int32_t i = 0; i < 2; ++i) {
            float new_min[3];
            float new_max[3];

            memcpy(new_min, min, sizeof(float) * 3);
            memcpy(new_max, max, sizeof(float) * 3);

            if (i == 0) {
                new_max[split_index] = mean[split_index];
            } else {
                new_min[split_index] = mean[split_index];
            }

            split_vertices(new_min, new_max, offset, (int32_t)indices[i + 1].size());
            offset += (int32_t)indices[i + 1].size();
        }
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

    float min[3]{ FLT_MAX };
    float max[3]{ FLT_MIN };

    auto extendWithVertex = [&min, &max](const aiVector3D& vertex) {
        for (int32_t i = 0; i < 3; ++i) {
            min[i] = std::min(min[i], vertex[i]);
            max[i] = std::max(max[i], vertex[i]);
        }
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
        meshes_.back().initialize(mesh->mMaterialIndex, indices, vertices, min, max);
    }
}
