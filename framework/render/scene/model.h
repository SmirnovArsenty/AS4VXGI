#pragma once

#include <string>
#include <vector>

#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "render/resource/shader.h"
#include "render/resource/buffer.h"

class Model
{
public:
    Model(const std::string& filename);

    void load();
    void unload();

    void set_position(Vector3 position);
    void set_scale(Vector3 scale);
    void set_rotation(Quaternion rotation);

    Vector3 position();
    Vector3 scale();
    Quaternion rotation();

    void draw();

    Vector3 extent_min();
    Vector3 extent_max();
    float radius();

private:
    // https://github.com/assimp/assimp/blob/master/samples/SimpleTexturedDirectx11/SimpleTexturedDirectx11/ModelLoader.cpp
    void load_node(aiNode* node, const aiScene* scene);
    void load_mesh(aiMesh* mesh, const aiScene* scene);

    const std::string filename_; // model filename

    std::vector<class Mesh*> meshes_;

    struct UniformData {
        Matrix transform;
        Matrix inverse_transpose_transform;
    } uniform_data_;
    DynamicBuffer<UniformData> uniform_buffer_;

    // model extents
    Vector3 min_;
    Vector3 max_;
};
