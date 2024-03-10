#include "render/common.h"

#include "core/game.h"
#include "render/render.h"
// #include "render/resource/shader_cache.h"
#include "render/scene/light.h"

PointLight::PointLight(const Vector3& color, const Vector3& position, float radius) :
    color_{ color }, position_{ position }, radius_{ radius }
{
}

void PointLight::initialize()
{
//     std::string cascade_count_str = std::to_string(Light::shadow_cascade_count);
//     D3D_SHADER_MACRO macro[] = {
//         "CASCADE_COUNT", cascade_count_str.c_str(),
//         nullptr, nullptr
//     };
//     shader_.set_vs_shader_from_file("./resources/shaders/deferred/light_pass/point.hlsl", "VSMain", macro);
//     shader_.set_ps_shader_from_file("./resources/shaders/deferred/light_pass/point.hlsl", "PSMain", macro);
//     D3D11_INPUT_ELEMENT_DESC inputs[] =
//     {
//         { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//     };
//     shader_.set_input_layout(inputs, std::size(inputs));
// #ifndef NDEBUG
//     shader_.set_name("point");
// #endif
// 
//     constexpr uint32_t vertical_segments_count = 20;
//     constexpr uint32_t horizontal_segments_count = 20;
// 
//     for (int32_t i = 0; i < vertical_segments_count; ++i) {
//         float vertical_angle = i * 3.14159265359f / vertical_segments_count;
//         float next_vertical_angle = (i + 1) * 3.14159265359f / vertical_segments_count;
//         for (int32_t j = 0; j < horizontal_segments_count; ++j) {
//             float horizontal_angle = j * 2 * 3.14159265359f / horizontal_segments_count;
//             float next_horizontal_angle = (j + 1) * 2 * 3.14159265359f / horizontal_segments_count;
//             if (i == 0) {
//                 Vector3 pos1{ sinf(vertical_angle) * cosf(horizontal_angle), cosf(vertical_angle), sinf(vertical_angle) * sinf(horizontal_angle) };
//                 Vector3 pos2{ sinf(next_vertical_angle) * cosf(horizontal_angle), cosf(next_vertical_angle), sinf(next_vertical_angle) * sinf(horizontal_angle) };
//                 Vector3 pos3{ sinf(next_vertical_angle) * cosf(next_horizontal_angle), cosf(next_vertical_angle), sinf(next_vertical_angle) * sinf(next_horizontal_angle) };
//                 indices_.push_back(uint32_t(vertices_.size()));
//                 vertices_.push_back(pos1);
//                 indices_.push_back(uint32_t(vertices_.size()));
//                 vertices_.push_back(pos2);
//                 indices_.push_back(uint32_t(vertices_.size()));
//                 vertices_.push_back(pos3);
//             } else if (i == vertical_segments_count - 1) {
//                 Vector3 pos1{ sinf(next_vertical_angle) * cosf(horizontal_angle), cosf(next_vertical_angle), sinf(next_vertical_angle) * cosf(horizontal_angle) };
//                 Vector3 pos2{ sinf(vertical_angle) * cosf(horizontal_angle), cosf(vertical_angle), sinf(vertical_angle) * sinf(horizontal_angle) };
//                 Vector3 pos3{ sinf(vertical_angle) * cosf(next_horizontal_angle), cosf(vertical_angle), sinf(vertical_angle) * sinf(next_horizontal_angle) };
//                 indices_.push_back(uint32_t(vertices_.size()));
//                 vertices_.push_back(pos1);
//                 indices_.push_back(uint32_t(vertices_.size()));
//                 vertices_.push_back(pos2);
//                 indices_.push_back(uint32_t(vertices_.size()));
//                 vertices_.push_back(pos3);
//             } else {
//                 uint32_t index_offset = uint32_t(vertices_.size());
//                 Vector3 pos1{ sinf(vertical_angle) * cosf(horizontal_angle), cosf(vertical_angle), sinf(vertical_angle) * sinf(horizontal_angle) };
//                 Vector3 pos2{ sinf(next_vertical_angle) * cosf(next_horizontal_angle), cosf(next_vertical_angle), sinf(next_vertical_angle) * sinf(next_horizontal_angle) };
//                 Vector3 pos3{ sinf(vertical_angle) * cosf(next_horizontal_angle), cosf(vertical_angle), sinf(vertical_angle) * sinf(next_horizontal_angle) };
//                 Vector3 pos4{ sinf(next_vertical_angle) * cosf(horizontal_angle), cosf(next_vertical_angle), sinf(next_vertical_angle) * sinf(horizontal_angle) };
//                 vertices_.push_back(pos1);
//                 vertices_.push_back(pos2);
//                 vertices_.push_back(pos3);
//                 vertices_.push_back(pos4);
//                 indices_.push_back(index_offset + 0);
//                 indices_.push_back(index_offset + 1);
//                 indices_.push_back(index_offset + 2);
//                 indices_.push_back(index_offset + 0);
//                 indices_.push_back(index_offset + 1);
//                 indices_.push_back(index_offset + 3);
//             }
//         }
//     }
// 
//     vertex_buffer_.initialize(vertices_.data(), vertices_.size(), sizeof(vertices_[0]));
//     index_buffer_.initialize(indices_.data(), indices_.size());
// 
//     point_buffer_.initialize(&point_data_);
// 
//     shader_.attach_buffer(1U, &point_buffer_);
//     shader_.attach_vertex_buffer(&vertex_buffer_);
//     shader_.attach_index_buffer(&index_buffer_);
// 
//     auto device = Game::inst()->render().device();
//     CD3D11_RASTERIZER_DESC rast_desc = {};
//     rast_desc.CullMode = D3D11_CULL_NONE;
//     rast_desc.FillMode = D3D11_FILL_SOLID;
//     rast_desc.FrontCounterClockwise = true;
//     D3D11_CHECK(device->CreateRasterizerState(&rast_desc, &rasterizer_state_));
}

void PointLight::destroy_resources()
{
//     point_buffer_.destroy();
//     vertex_buffer_.destroy();
//     index_buffer_.destroy();
//     shader_.destroy();
//     SAFE_RELEASE(rasterizer_state_);
}

void PointLight::draw()
{
    // calc shadows
    /// TODO

//     shader_.use();
//     auto context = Game::inst()->render().context();
//     context->RSSetState(rasterizer_state_);
//     context->DrawIndexed(UINT(indices_.size()), 0, 0);
}

void PointLight::update()
{
//     point_data_.transform = Matrix::CreateScale(radius_) * Matrix::CreateTranslation(position_);
//     point_data_.color = Vector4(color_.x, color_.y, color_.z, 0.f);
//     point_data_.position_radius = Vector4(position_.x, position_.y, position_.z, radius_);
//     point_buffer_.update(&point_data_);
}
