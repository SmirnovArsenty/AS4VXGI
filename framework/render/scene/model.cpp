#include <cassert>
#include <limits>

#include <assimp/Importer.hpp>

#define NOMINMAX

#include "core/game.h"
#include "render/render.h"
#include "render/camera.h"
#include "render/annotation.h"
#include "render/resource/texture.h"
#include "model.h"
#include "mesh.h"
#include "render/d3d11_common.h"

// public
Model::Model(const std::string& filename) :
    filename_{ filename },
    meshes_{},
    uniform_data_{ Matrix::Identity, Matrix::Identity },
    min_{ std::numeric_limits<float>::max() },
    max_{ std::numeric_limits<float>::min() }
{
}

void Model::load()
{
    // check not initialized
    assert(meshes_.empty());

    Assimp::Importer importer;
    auto scene = importer.ReadFile(filename_, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
    assert(scene != nullptr);
    load_node(scene->mRootNode, scene);

    // centrate all meshes
    for (auto& mesh : meshes_)
    {
        mesh->centrate((max_ + min_) / 2);
        mesh->initialize();
    }

    uniform_buffer_.initialize(sizeof(uniform_data_), D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void Model::unload()
{
    // check initialized
    assert(!meshes_.empty());

    uniform_buffer_.destroy();

    for (auto& mesh : meshes_) {
        mesh->destroy();
        delete mesh;
    }
    meshes_.clear();
}

void Model::set_position(Vector3 in_position)
{
    uniform_data_.transform = Matrix::CreateFromQuaternion(rotation()) * Matrix::CreateScale(scale()) * Matrix::CreateTranslation(in_position);
    uniform_data_.inverse_transpose_transform = uniform_data_.transform.Invert().Transpose();
}

void Model::set_scale(Vector3 in_scale)
{
    uniform_data_.transform = Matrix::CreateFromQuaternion(rotation()) * Matrix::CreateScale(in_scale) * Matrix::CreateTranslation(position());
    uniform_data_.inverse_transpose_transform = uniform_data_.transform.Invert().Transpose();
}

void Model::set_rotation(Quaternion in_rotation)
{
    uniform_data_.transform = Matrix::CreateFromQuaternion(in_rotation) * Matrix::CreateScale(scale()) * Matrix::CreateTranslation(position());
    uniform_data_.inverse_transpose_transform = uniform_data_.transform.Invert().Transpose();
}

Vector3 Model::position()
{
    Vector3 ret_position;
    Vector3 ret_scale;
    Quaternion ret_rotation;
    uniform_data_.transform.Decompose(ret_scale, ret_rotation, ret_position);
    return ret_position;
}

Vector3 Model::scale()
{
    Vector3 ret_position;
    Vector3 ret_scale;
    Quaternion ret_rotation;
    uniform_data_.transform.Decompose(ret_scale, ret_rotation, ret_position);
    return ret_scale;
}

Quaternion Model::rotation()
{
    Vector3 ret_position;
    Vector3 ret_scale;
    Quaternion ret_rotation;
    uniform_data_.transform.Decompose(ret_scale, ret_rotation, ret_position);
    return ret_rotation;
}

void Model::draw()
{
    Annotation annotation("draw:" + filename_);

    uniform_buffer_.update_data(&uniform_data_);
    uniform_buffer_.bind(1);

    for (auto& mesh : meshes_) {
        mesh->draw();
    }
}

Vector3 Model::extent_min()
{
    Vector3 ret_position;
    Vector3 ret_scale;
    Quaternion ret_rotation;
    uniform_data_.transform.Decompose(ret_scale, ret_rotation, ret_position);
    return min_ * ret_scale;
}

Vector3 Model::extent_max()
{
    Vector3 ret_position;
    Vector3 ret_scale;
    Quaternion ret_rotation;
    uniform_data_.transform.Decompose(ret_scale, ret_rotation, ret_position);
    return max_ * ret_scale;
}

float Model::radius()
{
    Vector3 diag = extent_max() - extent_min();
    return diag.Length() / 2;
}

// private
void Model::load_node(aiNode* node, const aiScene* scene)
{
    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        auto mesh = scene->mMeshes[node->mMeshes[i]];
        load_mesh(mesh, scene);
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        load_node(node->mChildren[i], scene);
    }
}

void Model::load_mesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
        Vertex vertex;

        vertex.position_uv_x.x = mesh->mVertices[i].x;
        vertex.position_uv_x.y = mesh->mVertices[i].y;
        vertex.position_uv_x.z = mesh->mVertices[i].z;

        vertex.normal_uv_y.x = mesh->mNormals[i].x;
        vertex.normal_uv_y.y = mesh->mNormals[i].y;
        vertex.normal_uv_y.z = mesh->mNormals[i].z;

        if (mesh->mTextureCoords[0]) {
            vertex.position_uv_x.w = (float)mesh->mTextureCoords[0][i].x;
            vertex.normal_uv_y.w = (float)mesh->mTextureCoords[0][i].y;
        }

        vertices.push_back(vertex);

        // update extents
        if (min_.x > mesh->mVertices[i].x) {
            min_.x = mesh->mVertices[i].x;
        }
        if (min_.y > mesh->mVertices[i].y) {
            min_.y = mesh->mVertices[i].y;
        }
        if (min_.z > mesh->mVertices[i].z) {
            min_.z = mesh->mVertices[i].z;
        }

        if (max_.x < mesh->mVertices[i].x) {
            max_.x = mesh->mVertices[i].x;
        }
        if (max_.y < mesh->mVertices[i].y) {
            max_.y = mesh->mVertices[i].y;
        }
        if (max_.z < mesh->mVertices[i].z) {
            max_.z = mesh->mVertices[i].z;
        }
    }

    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        auto& face = mesh->mFaces[i];

        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Material* material = nullptr;

    if (mesh->mMaterialIndex >= 0) {
        auto mat = scene->mMaterials[mesh->mMaterialIndex];
        material = new Material(mat->GetName().C_Str());

        auto load_texture = [this, &scene](aiString& str) -> Texture* {
            auto texture = new Texture();
            auto embedded_texture = scene->GetEmbeddedTexture(str.C_Str());
            if (embedded_texture != nullptr) {
                if (embedded_texture->mHeight != 0) {
                    texture->initialize(embedded_texture->mWidth, embedded_texture->mHeight, DXGI_FORMAT_B8G8R8A8_UNORM, embedded_texture->pcData);
                }
            } else {
                auto model_path = filename_.substr(0, filename_.find_last_of('/') + 1);
                texture->load((model_path + str.C_Str()).c_str());
            }
            return texture;
        };

        // diffuse
        for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); ++i) {
            aiString str;
            mat->GetTexture(aiTextureType_DIFFUSE, i, &str);
            material->set_diffuse(load_texture(str));
        }

        // specular
        for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_SPECULAR); ++i) {
            aiString str;
            mat->GetTexture(aiTextureType_SPECULAR, i, &str);
            material->set_specular(load_texture(str));
        }

        // ambient
        for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_AMBIENT); ++i) {
            aiString str;
            mat->GetTexture(aiTextureType_AMBIENT, i, &str);
            material->set_ambient(load_texture(str));
        }
    }

    {
        material->initialize();
        auto new_mesh = new Mesh(vertices, indices, material);
        // new_mesh->initialize();
        meshes_.push_back(new_mesh);
    }
}
