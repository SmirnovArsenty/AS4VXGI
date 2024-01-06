#define NOMINMAX

#include <functional>
#include <assimp/Importer.hpp>

#include "core/game.h"
#include "render/render.h"
#include "render/camera.h"
#include "model_tree.h"

constexpr float smallest_length = 0.5f;

void ModelTree::Mesh::initialize(Material& material,
    const std::vector<uint32_t>& indices,
    const std::vector<Vertex>& vertices,
    float min[3], float max[3])
{
    material_ = material;
    for (int32_t i = 0; i < _countof(min_); ++i) {
        min_[i] = min[i];
        max_[i] = max[i];
    }
    indices_ = indices;
    vertices_ = vertices;
    for (uint32_t index : indices_) {
        assert(index < vertices_.size());
    }
    index_count_ = static_cast<UINT>(indices.size());

    mesh_tree_.reserve(index_count_ / 3);
    split_vertices(min_, max_, 0, index_count_, 0);
    uint32_t max_index = 0;
    for (auto& node : mesh_tree_) {
        max_index = std::max<uint32_t>(node.first, max_index);
    }

    // 0 - root index, already present
    // max_index - last present index, don't check it
    for (uint32_t i = 1; i < max_index; ++i) {
        auto it = mesh_tree_.find(i);
        if (it == mesh_tree_.end()) {
            uint32_t parent_index = (i - 1) / 2;
            MeshTreeNode parent = mesh_tree_[parent_index];

            const float length[3] = { max[0] - min[0], max[1] - min[1], max[2] - min[2] };
            const float mean[3] = { (max[0] + min[0]) / 2, (max[1] + min[1]) / 2, (max[2] + min[2]) / 2 };
            int32_t split_index = -1;
            if (length[0] >= length[1] && length[0] >= length[2] && length[0] > smallest_length) {
                split_index = 0;
            }
            else if (length[1] >= length[0] && length[1] >= length[2] && length[1] > smallest_length) {
                split_index = 1;
            }
            else if (length[2] >= length[0] && length[2] >= length[1] && length[2] > smallest_length) {
                split_index = 2;
            }

            aiVector3D node_min;
            aiVector3D node_max;
            memcpy(&node_min, &parent.min, sizeof(float) * 3);
            memcpy(&node_max, &parent.max, sizeof(float) * 3);

            if (i & 1) {
                node_max[split_index] = mean[split_index];
            } else {
                node_min[split_index] = mean[split_index];
            }

            MeshTreeNode node{};
            memcpy(&node.min, &node_min, sizeof(float) * 3);
            memcpy(&node.max, &node_max, sizeof(float) * 3);
            node.start_index = 0;
            node.count = 0;

            mesh_tree_[i] = node;
        }
    }

    index_buffer_.initialize(indices.data(), indices.size());
    vertex_buffer_.initialize(vertices.data(), vertices.size(), sizeof(Vertex));

    index_buffer_resource_view_.initialize(indices.data(), indices.size());
    vertex_buffer_resource_view_.initialize(vertices.data(), vertices.size());

#ifndef NDEBUG
    box_shader_.set_vs_shader_from_file("./resources/shaders/debug/box.hlsl", "VSMain");
    box_shader_.set_ps_shader_from_file("./resources/shaders/debug/box.hlsl", "PSMain");
    D3D11_INPUT_ELEMENT_DESC inputs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    box_shader_.set_input_layout(inputs, std::size(inputs));

    std::vector<uint32_t> box_indices;
    std::vector<Vector4> box_vertices;

    // box with size of 1
    box_vertices.push_back({ .5, .5, .5, 1 }); // 0
    box_vertices.push_back({ .5, .5, -.5, 1 }); // 1
    box_vertices.push_back({ .5, -.5, .5, 1 }); // 2
    box_vertices.push_back({ -.5, .5, .5, 1 }); // 3
    box_vertices.push_back({ .5, -.5, -.5, 1 }); // 4
    box_vertices.push_back({ -.5, .5, -.5, 1 }); // 5
    box_vertices.push_back({ -.5, -.5, .5, 1 }); // 6
    box_vertices.push_back({ -.5, -.5, -.5, 1 }); // 7

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

    box_index_buffer_.initialize(box_indices.data(), box_indices.size());
    box_vertex_buffer_.initialize(box_vertices.data(), box_vertices.size(), sizeof(Vector4));

    std::vector<Matrix> box_transformations;
    for (int32_t i = 0; i < mesh_tree_.size(); ++i) {
        Vector3 position;
        Vector3 scale;

        position.x = (mesh_tree_[i].max.x + mesh_tree_[i].min.x) / 2;
        position.y = (mesh_tree_[i].max.y + mesh_tree_[i].min.y) / 2;
        position.z = (mesh_tree_[i].max.z + mesh_tree_[i].min.z) / 2;
        scale.x = (mesh_tree_[i].max.x - mesh_tree_[i].min.x);
        scale.y = (mesh_tree_[i].max.y - mesh_tree_[i].min.y);
        scale.z = (mesh_tree_[i].max.z - mesh_tree_[i].min.z);

        // scale *= 1.1f;

        box_transformations.push_back(Matrix::CreateScale(scale.x, scale.y, scale.z) * Matrix::CreateTranslation(position.x, position.y, position.z));
    }
    box_transformations_.initialize(box_transformations.data(), (int32_t)std::size(box_transformations));

    box_shader_.attach_index_buffer(&box_index_buffer_);
    box_shader_.attach_vertex_buffer(&box_vertex_buffer_);
    box_shader_.attach_resource(0, &box_transformations_);
#endif
}

void ModelTree::Mesh::destroy()
{
    material_.destroy();
    index_count_ = 0;
    index_buffer_.destroy();
    vertex_buffer_.destroy();
    index_buffer_resource_view_.destroy();
    vertex_buffer_resource_view_.destroy();

#ifndef NDEBUG
    box_index_buffer_.destroy();
    box_vertex_buffer_.destroy();
    box_transformations_.destroy();
    box_shader_.destroy();
#endif
}

void ModelTree::Mesh::draw()
{
    index_buffer_.bind();
    vertex_buffer_.bind(0U);
    material_.use();

    auto context = Game::inst()->render().context();
    context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(index_count_, 0, 0);
}

#ifndef NDEBUG
void ModelTree::Mesh::debug_draw()
{
    box_shader_.use();

    auto context = Game::inst()->render().context();
    context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    context->DrawIndexedInstanced(24, (int32_t)std::size(mesh_tree_), 0, 0, 0);
}
#endif

void ModelTree::load(const std::string& file)
{
    Assimp::Importer importer;
    auto scene = importer.ReadFile(file, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_GenUVCoords);
    assert(scene != nullptr);
    meshes_.reserve(scene->mNumMeshes);
    load_node(scene->mRootNode, scene);

    D3D11_INPUT_ELEMENT_DESC inputs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD_U", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD_V", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    albedo_shader_.set_vs_shader_from_file("./resources/shaders/debug/albedo.hlsl", "VSMain");
    albedo_shader_.set_ps_shader_from_file("./resources/shaders/debug/albedo.hlsl", "PSMain");
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
    SAFE_RELEASE(rasterizer_state_);

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
    context->RSSetState(rasterizer_state_);

    // albedo_shader_.use();
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

std::vector<ID3D11ShaderResourceView*> ModelTree::get_index_buffers_srv()
{
    std::vector<ID3D11ShaderResourceView*> result;
    result.reserve(meshes_.size());
    for (Mesh& mesh : meshes_) {
        result.push_back(mesh.get_index_buffer_srv());
    }
    return result;
}

std::vector<ID3D11ShaderResourceView*> ModelTree::get_vertex_buffers_srv()
{
    std::vector<ID3D11ShaderResourceView*> result;
    result.reserve(meshes_.size());
    for (Mesh& mesh : meshes_) {
        result.push_back(mesh.get_vertex_buffer_srv());
    }
    return result;
}

std::vector<std::vector<MeshTreeNode>> ModelTree::get_meshes_trees()
{
    std::vector<std::vector<MeshTreeNode>> result;
    result.reserve(meshes_.size());
    for (Mesh& mesh : meshes_) {
        result.push_back(mesh.get_mesh_tree());
    }
    return result;
}

std::vector<MeshTreeNode> ModelTree::Mesh::get_mesh_tree() const
{
    std::vector<MeshTreeNode> res;
    res.reserve(mesh_tree_.size());
    for (int i = 0; i < mesh_tree_.size(); ++i) {
        res.push_back(mesh_tree_.at(i));
    }
    return res;
}

void ModelTree::Mesh::split_vertices(float min[3], float max[3], int32_t start, int32_t count, int32_t current_mesh_node_index)
{
    if (count == 0) {
        return;
    }

    const float length[3] = { max[0] - min[0], max[1] - min[1], max[2] - min[2] };
    const float mean[3] = { (max[0] + min[0]) / 2, (max[1] + min[1]) / 2, (max[2] + min[2]) / 2 };
    int32_t split_index = -1;
    if (length[0] >= length[1] && length[0] >= length[2] && length[0] > smallest_length) {
        split_index = 0;
    } else if (length[1] >= length[0] && length[1] >= length[2] && length[1] > smallest_length) {
        split_index = 1;
    } else if (length[2] >= length[0] && length[2] >= length[1] && length[2] > smallest_length) {
        split_index = 2;
    }

    // radix sort

    std::vector<int32_t> indices[3]; // 0 - parent, 1 - less child, 2 - greater child
    for (int32_t i = 0; i < 3; ++i) {
        indices[i].reserve(count);
    }
    for (int32_t i = start; i < start + count; i += 3) {
        uint32_t triangle_indices[3] = { indices_[i + 0], indices_[i + 1], indices_[i + 2] };
        float triangle[3][3] = {
            { vertices_[triangle_indices[0]].position.x, vertices_[triangle_indices[0]].position.y, vertices_[triangle_indices[0]].position.z },
            { vertices_[triangle_indices[1]].position.x, vertices_[triangle_indices[1]].position.y, vertices_[triangle_indices[1]].position.z },
            { vertices_[triangle_indices[2]].position.x, vertices_[triangle_indices[2]].position.y, vertices_[triangle_indices[2]].position.z }
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

    int32_t indices_count[3] = { (int32_t)indices[0].size(), (int32_t)indices[1].size(), (int32_t)indices[2].size() };

    //if (indices_count[0] > 0) {
        MeshTreeNode node;
        node.min = Vector3(min);
        node.max = Vector3(max);
        node.start_index = start;
        node.count = indices_count[0];
        mesh_tree_[current_mesh_node_index] = node;
    //}

    {
        int32_t offset = start;
        for (int32_t i = 0; i < 3; ++i) {
            if (indices_count[i] != 0) {
                memcpy(&indices_[offset], indices[i].data(), indices_count[i] * sizeof(uint32_t));
            }
            offset += indices_count[i];
        }
    }

    if (split_index != -1) {
        int32_t offset = start + indices_count[0];
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

            // children:
            // (2i + 1) and (2i + 2)
            // parent:
            // (i - 1) / 2
            split_vertices(new_min, new_max, offset, indices_count[i + 1], 2 * current_mesh_node_index + 1 + i);
            offset += indices_count[i + 1];
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
    std::vector<Vertex> vertices;

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
        vertices.push_back({ Vector3(&mesh->mVertices[i].x),
                            mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y,
                            Vector3(&mesh->mNormals[i].x) });

        extendWithVertex(mesh->mVertices[i]);
    }

    // fill indices
    uint32_t vertices_count = (uint32_t)vertices.size();
    indices.reserve(mesh->mNumFaces * 3);
    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        auto& face = mesh->mFaces[i];
        assert(face.mNumIndices == 3);
        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            assert(face.mIndices[j] < vertices_count);
            indices.push_back(face.mIndices[j]);
        }
    }

    Material material;
    {
        Sampler* sampler = new Sampler();
        sampler->initialize();
        material.set_sampler(sampler);
    }
    if (mesh->mMaterialIndex >= 0) {
        auto mat = scene->mMaterials[mesh->mMaterialIndex];

        auto load_texture = [this, &scene](aiString& str) -> Texture2D* {
            Texture2D* texture = new Texture2D();
            const aiTexture* embedded_texture = scene->GetEmbeddedTexture(str.C_Str());
            if (embedded_texture != nullptr) {
                if (embedded_texture->mHeight != 0) {
                    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
                    if (strcmp(embedded_texture->achFormatHint, "rgba8888") == 0) {
                        format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    } else if (strcmp(embedded_texture->achFormatHint, "argb8888") == 0) {
                        format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    } else if (strcmp(embedded_texture->achFormatHint, "bgra5650") == 0) {
                        format = DXGI_FORMAT_B5G6R5_UNORM;
                    }
                    texture->initialize(embedded_texture->mWidth, embedded_texture->mHeight, format, embedded_texture->pcData);
                }
            } else {
                return nullptr;
                // auto model_path = filename_.substr(0, filename_.find_last_of('/') + 1);
                // texture->load((model_path + str.C_Str()).c_str());
            }
            return texture;
        };
    
        // diffuse
        for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); ++i) {
            aiString str;
            mat->GetTexture(aiTextureType_DIFFUSE, i, &str);
            material.set_diffuse(load_texture(str));
        }
    
        //// specular
        //for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_SPECULAR); ++i) {
        //    aiString str;
        //    mat->GetTexture(aiTextureType_SPECULAR, i, &str);
        //    material.set_specular(load_texture(str));
        //}
        //
        //// ambient
        //for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_AMBIENT); ++i) {
        //    aiString str;
        //    mat->GetTexture(aiTextureType_AMBIENT, i, &str);
        //    material.set_ambient(load_texture(str));
        //}
    }

    {
        meshes_.push_back({});
        meshes_.back().initialize(material, indices, vertices, min, max);
    }
}
