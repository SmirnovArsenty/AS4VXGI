#include <windows.h>
#include <chrono>
#include <cmath>
#include <sstream>


#include "core/game.h"
#include "win32/win.h"
#include "camera.h"

#include "render/common.h"
#include "render/render.h"
#include "render/scene/light.h"

Camera::Camera()
{
    camera_data_.cameraData.position = { -20.f, 0.f, 0.f };
    camera_data_.cameraData.forward = { 1.f, 0.f, 0.f };
}

Camera::~Camera()
{
}

void Camera::initialize()
{
    camera_data_cb_.initialize();
}

void Camera::destroy()
{
}

void Camera::update()
{
    if (durty_) {
        // update matrices
        camera_data_.cameraData.vp = view_proj();
        camera_data_.cameraData.vp_inverse = view_proj().Invert();
        camera_data_.cameraData.projection = proj();
        camera_data_.cameraData.projection_inverse = proj().Invert();
        camera_data_.cameraData.screen_width = Game::inst()->win().screen_width();
        camera_data_.cameraData.screen_height = Game::inst()->win().screen_height();

        camera_data_cb_.update(camera_data_);

        durty_ = false;
    }
}

float Camera::get_near() const
{
    constexpr float near_plane = .1f;
    return near_plane;
}

float Camera::get_far() const
{
    constexpr float far_plane = 1e5f;
    return far_plane;
}

float Camera::get_fov() const
{
    float width = Game::inst()->win().screen_width();
    float height = Game::inst()->win().screen_height();
    float aspect_ratio = width / height;
    constexpr float fov = 60 / 57.2957795131f; // default fov - 60 degree
    auto vfov = static_cast<float>(2 * atan(tan(fov / 2) * (1.0 / aspect_ratio)));
    float field_of_view = aspect_ratio > 1.0f ? fov : vfov;
    return field_of_view;
}

void Camera::set_camera(Vector3 position, Vector3 forward)
{
    durty_ = true;
    camera_data_.cameraData.position = position;
    forward.Normalize(camera_data_.cameraData.forward);

    if (focus_) {
        camera_data_.cameraData.forward = focus_target_ - camera_data_.cameraData.position;
        focus_distance_ = sqrt(camera_data_.cameraData.forward.Dot(camera_data_.cameraData.forward));
        camera_data_.cameraData.forward.Normalize();
    }
}

void Camera::pitch(float delta) // vertical
{
    durty_ = true;
    Vector3 up(0.f, 1.f, 0.f);
    Vector3 right = camera_data_.cameraData.forward.Cross(up);
    up = camera_data_.cameraData.forward.Cross(right);

    if (focus_) {
        camera_data_.cameraData.forward -= up * delta;
    } else {
        camera_data_.cameraData.forward += up * delta;
    }
    camera_data_.cameraData.forward.Normalize();
    if (focus_) {
        camera_data_.cameraData.position = focus_target_ - camera_data_.cameraData.forward * focus_distance_;
    }
}

void Camera::yaw(float delta) // horizontal
{
    durty_ = true;
    Vector3 up(0.f, 1.f, 0.f);
    Vector3 right = up.Cross(camera_data_.cameraData.forward);

    if (focus_) {
        camera_data_.cameraData.forward -= right * delta;
    } else {
        camera_data_.cameraData.forward += right * delta;
    }
    camera_data_.cameraData.forward.Normalize();
    if (focus_) {
        camera_data_.cameraData.position = focus_target_ - camera_data_.cameraData.forward * focus_distance_;
    }
}

Camera::CameraType Camera::type() const
{
    return type_;
}

void Camera::set_type(Camera::CameraType type)
{
    type_ = type;
}

void Camera::focus(Vector3 target, float min_distance)
{
    durty_ = true;
    focus_target_ = target;
    camera_data_.cameraData.forward = focus_target_ - camera_data_.cameraData.position;
    camera_data_.cameraData.forward.Normalize();
    if (!focus_)
    {
        focus_min_distance_ = abs(min_distance);
        focus_distance_ = (camera_data_.cameraData.position - target).Length();
        if (focus_distance_ < focus_min_distance_) {
            focus_distance_ = focus_min_distance_;
        }
    }
    camera_data_.cameraData.position = focus_target_ - camera_data_.cameraData.forward * focus_distance_;

    focus_ = true;
}

void Camera::reset_focus()
{
    focus_ = false;
}

const Matrix Camera::view() const
{
    Vector3 pos = camera_data_.cameraData.position;
    if (focus_) {
        pos = focus_target_ - camera_data_.cameraData.forward * focus_distance_;
    }
    return Matrix::CreateLookAt(pos, pos + direction(), Vector3(0, 1, 0));
}

const Matrix Camera::proj() const
{
    float width = Game::inst()->win().screen_width();
    float height = Game::inst()->win().screen_height();

    switch (type_)
    {
        case CameraType::perspective:
        {
            float aspect_ratio = width / height;
            return Matrix::CreatePerspectiveFieldOfView(get_fov(), aspect_ratio, get_near(), get_far());
        }
        case CameraType::orthographic:
        {
            return Matrix::CreateOrthographic(width, height, get_near(), get_far());
        }
        default:
        {
            break;
        }
    }
    return Matrix::Identity;
}

const Matrix Camera::view_proj() const
{
    return view() * proj();
}

std::vector<std::pair<Matrix, float>> Camera::cascade_view_proj()
{
    std::vector<std::pair<Matrix, float>> res;
    res.reserve(Light::shadow_cascade_count);

    float fars[Light::shadow_cascade_count] = { 50.f, 200.f, get_far() };
    float nears[Light::shadow_cascade_count] = { get_near(), get_near(), get_near() };
    for (uint32_t i = 0; i < Light::shadow_cascade_count; ++i)
    {
        float width = Game::inst()->win().screen_width();
        float height = Game::inst()->win().screen_height();
        Matrix projection = Matrix::Identity;
        switch (type_)
        {
        case CameraType::perspective:
        {
            float aspect_ratio = width / height;
            projection = Matrix::CreatePerspectiveFieldOfView(get_fov(), aspect_ratio, nears[i], fars[i]);
            break;
        }
        case CameraType::orthographic:
        {
            projection = Matrix::CreateOrthographic(width, height, nears[i], fars[i]);
            break;
        }
        default:
        {
            break;
        }
        }
        res.push_back(std::make_pair(view() * projection, fars[i]));
    }
    return res;
}

const Vector3& Camera::position() const
{
    return camera_data_.cameraData.position;
}

const Vector3& Camera::direction() const
{
    return camera_data_.cameraData.forward;
}

Vector3 Camera::right() const
{
    Vector3 up(0.f, 1.f, 0.f);
    return camera_data_.cameraData.forward.Cross(up);
}

Vector3 Camera::up() const
{
    Vector3 up(0.f, 1.f, 0.f);
    Vector3 right = camera_data_.cameraData.forward.Cross(up);
    return camera_data_.cameraData.forward.Cross(right);
}

void Camera::move_forward(float delta)
{
    durty_ = true;
    if (focus_)
    {
        focus_distance_ -= delta;
        if (focus_distance_ < focus_min_distance_) {
            focus_distance_ = focus_min_distance_;
        }
        camera_data_.cameraData.position = focus_target_ - camera_data_.cameraData.forward * focus_distance_;
    }
    else
    {
        camera_data_.cameraData.position += camera_data_.cameraData.forward * delta;
    }
}

void Camera::move_right(float delta)
{
    durty_ = true;

    Vector3 up(0.f, 1.f, 0.f);
    Vector3 right = camera_data_.cameraData.forward.Cross(up);
    right.Normalize();

    if (!focus_) {
        camera_data_.cameraData.position += right * delta;
    }
}

void Camera::move_up(float delta)
{
    if (!focus_) {
        camera_data_.cameraData.position += Vector3(0.f, delta, 0.f);
    }
}
