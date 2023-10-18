#include "core/game.h"
#include "render/render.h"
#include "mesh.h"

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, Material* material) :
    vertices_{ vertices }, indices_{ indices }, material_{ material }, uniform_data_{}
{
}

Mesh::~Mesh()
{
    if (material_ != nullptr) {
        material_->destroy();
        delete material_;
        material_ = nullptr;
    }
}

void Mesh::initialize()
{
    vertex_buffer_.initialize(vertices_.data(), vertices_.size(), sizeof(vertices_[0]));
    index_buffer_.initialize(indices_.data(), indices_.size());
#ifndef NDEBUG
    vertex_buffer_.set_name("vertex_buffer");
    index_buffer_.set_name("index_buffer");
#endif

    uniform_data_.material_flags = 0;
    if (material_->get_diffuse()) {
        uniform_data_.material_flags |= 1 << 0;
    }

    if (material_->get_specular()) {
        uniform_data_.material_flags |= 1 << 1;
    }

    if (material_->get_ambient()) {
        uniform_data_.material_flags |= 1 << 2;
    }

    uniform_buffer_.initialize(&uniform_data_);
}

void Mesh::destroy()
{
    uniform_buffer_.destroy();
    index_buffer_.destroy();
    vertex_buffer_.destroy();
}

void Mesh::draw()
{
    auto context = Game::inst()->render().context();

    context->PSSetConstantBuffers(2U, 1, &uniform_buffer_.getBuffer());
    context->VSSetConstantBuffers(2U, 1, &uniform_buffer_.getBuffer());

    vertex_buffer_.bind(0U);
    index_buffer_.bind();

    material_->bind();

    context->DrawIndexed(static_cast<UINT>(indices_.size()), 0, 0);
}

void Mesh::centrate(Vector3 center)
{
    for (auto& vertex : vertices_)
    {
        vertex.position_uv_x.x -= center.x;
        vertex.position_uv_x.y -= center.y;
        vertex.position_uv_x.z -= center.z;
    }
}
