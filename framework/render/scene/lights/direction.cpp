#include "core/game.h"
#include "render/render.h"
#include "render/resource/shader_cache.h"
#include "render/scene/light.h"

DirectionLight::DirectionLight(const Vector3& color, const Vector3& direction)
    : color_{ color }, direction_{ direction }
{
    std::string cascade_count_str = std::to_string(Light::shadow_cascade_count);
    D3D_SHADER_MACRO macro[] = {
        "CASCADE_COUNT", cascade_count_str.c_str(),
        nullptr, nullptr
    };
    shader_ = Game::inst()->render().shader_cache()->add_shader(
        "./resources/shaders/deferred/light_pass/direction.hlsl",
        ShaderCache::ShaderFlags(ShaderCache::ShaderFlags::S_VERTEX | ShaderCache::ShaderFlags::S_PIXEL),
        macro);
#ifndef NDEBUG
    shader_->set_name("direction");
#endif
}

void DirectionLight::initialize()
{
    direction_buffer_.initialize(sizeof(direction_data_));
}

void DirectionLight::destroy_resources()
{
    direction_buffer_.destroy();
}

void DirectionLight::update()
{
    direction_data_.color.x = direction_.x;
    direction_data_.color.y = direction_.y;
    direction_data_.color.z = direction_.z;
    direction_data_.direction.x = color_.x;
    direction_data_.direction.y = color_.y;
    direction_data_.direction.z = color_.z;
    direction_buffer_.update_data(&direction_data_);
}

void DirectionLight::draw()
{
    // calc shadows
    /// TODO

    assert(shader_ != nullptr);
    shader_->use();
    direction_buffer_.bind(1U);
    Game::inst()->render().context()->Draw(3, 0);
}
