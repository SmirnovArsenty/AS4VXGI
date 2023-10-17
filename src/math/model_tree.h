#pragma once

#include <string>

#include <assimp/scene.h>
#include <assimp/postprocess.h>

class ModelTree
{
public:
    ModelTree();
    ~ModelTree();

    // load model to tree
    // allocate resources
    void load(const std::string& path);

    // destroy resources
    void unload();
private:
    void load_node(aiNode* node, const aiScene* scene);
    void load_mesh(aiMesh* mesh, const aiScene* scene);
};
