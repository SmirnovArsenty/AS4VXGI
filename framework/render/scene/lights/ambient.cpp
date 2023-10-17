#include "core/game.h"
#include "render/render.h"
// #include "render/resource/shader_cache.h"
#include "render/scene/light.h"

AmbientLight::AmbientLight(Vector3 color) : color_{ color }
{

}

void AmbientLight::initialize()
{
    shader_.set_vs_shader_from_file("./resources/shaders/deferred/light_pass/ambient.hlsl", "VSMain");
    shader_.set_ps_shader_from_file("./resources/shaders/deferred/light_pass/ambient.hlsl", "PSMain");
#ifndef NDEBUG
    shader_.set_name("ambient");
#endif
    ambient_buffer_.initialize(&ambient_data_);

    shader_.attach_buffer(1U, &ambient_buffer_);
}

void AmbientLight::destroy_resources()
{
    ambient_buffer_.destroy();
    shader_.destroy();
}

void AmbientLight::update()
{
    ambient_data_.color.x = color_.x;
    ambient_data_.color.y = color_.y;
    ambient_data_.color.z = color_.z;
    ambient_buffer_.update(&ambient_data_);
}

void AmbientLight::draw()
{
    shader_.use();

    Game::inst()->render().context()->Draw(3, 0);
}
