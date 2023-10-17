#include "core/game.h"
#include "render/render.h"
// #include "render/resource/shader_cache.h"
#include "render/scene/light.h"

DirectionLight::DirectionLight(const Vector3& color, const Vector3& direction)
    : color_{ color }, direction_{ direction }
{
}

void DirectionLight::initialize()
{
    std::string cascade_count_str = std::to_string(Light::shadow_cascade_count);
    D3D_SHADER_MACRO macro[] = {
        "CASCADE_COUNT", cascade_count_str.c_str(),
        nullptr, nullptr
    };
    shader_.set_vs_shader_from_file("./resources/shaders/deferred/light_pass/direction.hlsl", "VSMain", macro);
    shader_.set_ps_shader_from_file("./resources/shaders/deferred/light_pass/direction.hlsl", "PSMain", macro);
#ifndef NDEBUG
    shader_.set_name("direction");
#endif
    direction_buffer_.initialize(&direction_data_);
    shader_.attach_buffer(1, &direction_buffer_);
}

void DirectionLight::destroy_resources()
{
    direction_buffer_.destroy();
    shader_.destroy();
}

void DirectionLight::update()
{
    direction_data_.color.x = direction_.x;
    direction_data_.color.y = direction_.y;
    direction_data_.color.z = direction_.z;
    direction_data_.direction.x = color_.x;
    direction_data_.direction.y = color_.y;
    direction_data_.direction.z = color_.z;
    direction_buffer_.update(&direction_data_);
}

void DirectionLight::draw()
{
    // calc shadows
    /// TODO

    shader_.use();
    Game::inst()->render().context()->Draw(3, 0);
}
