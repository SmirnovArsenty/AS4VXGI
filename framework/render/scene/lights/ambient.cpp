#include "core/game.h"
#include "render/render.h"
#include "render/resource/shader_cache.h"
#include "render/scene/light.h"

AmbientLight::AmbientLight(Vector3 color) : color_{ color }
{
    shader_ = Game::inst()->render().shader_cache()->add_shader(
        "./resources/shaders/deferred/light_pass/ambient.hlsl",
        ShaderCache::ShaderFlags(ShaderCache::ShaderFlags::S_VERTEX | ShaderCache::ShaderFlags::S_PIXEL));
#ifndef NDEBUG
    shader_->set_name("ambient");
#endif
}

void AmbientLight::initialize()
{
    ambient_buffer_.initialize(sizeof(ambient_data_), D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void AmbientLight::destroy_resources()
{
    ambient_buffer_.destroy();
}

void AmbientLight::update()
{
    ambient_data_.color.x = color_.x;
    ambient_data_.color.y = color_.y;
    ambient_data_.color.z = color_.z;
    ambient_buffer_.update_data(&ambient_data_);
}

void AmbientLight::draw()
{
    assert(shader_ != nullptr);

    shader_->use();

    ambient_buffer_.bind(1U);

    Game::inst()->render().context()->Draw(3, 0);
}
