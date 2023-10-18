#pragma once

#include <vector>

#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

#include "render/resource/buffer.h"
#include "material.h"

struct Vertex
{
    Vector4 position_uv_x;
    Vector4 normal_uv_y;
};

class Mesh
{
public:
    Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, Material* material);
    ~Mesh();

    void initialize();
    void destroy();

    void draw();

    void centrate(Vector3 center);
private:
    std::vector<Vertex> vertices_;
    VertexBuffer vertex_buffer_;
    std::vector<uint32_t> indices_;
    IndexBuffer index_buffer_;
    Material* material_;

    struct UniformData
    {
        uint32_t material_flags;
        uint32_t dummy[3];
    } uniform_data_;
    DynamicBuffer<UniformData> uniform_buffer_;
};
