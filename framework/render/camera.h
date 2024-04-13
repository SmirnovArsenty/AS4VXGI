#pragma once

#include <cstdint>
#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

#include "shaders/common/types.fx"

#include "render/resource/buffer.hpp"

class Camera
{
public:
    enum class CameraType : uint32_t
    {
        perspective,
        orthographic,
    };

    Camera();
    ~Camera();

    void initialize();
    void destroy();
    void update();

    float get_near() const;
    float get_far() const;
    float get_fov() const;

    void set_camera(Vector3 position, Vector3 forward);

    void pitch(float delta); // around right vector
    void yaw(float delta); // around up vector

    CameraType type() const;
    void set_type(CameraType type);

    void focus(Vector3 target, float min_distance = 0.f);
    void reset_focus();

    const Matrix view() const;
    const Matrix proj() const;
    const Matrix view_proj() const;

    std::vector<std::pair<Matrix, float>> cascade_view_proj();

    const Vector3& position() const;
    const Vector3& direction() const;

    Vector3 right() const;
    Vector3 up() const;

    // update camera position
    void move_forward(float delta);
    void move_right(float delta);
    void move_up(float delta);

    const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_descriptor_handle() const
    {
        return camera_data_cb_.gpu_descriptor_handle();
    }

private:
    bool durty_{ true };

    CameraType type_{ CameraType::perspective };
    bool focus_{ true };
    Vector3 focus_target_{ 0.f, 0.f, 0.f };
    float focus_min_distance_{ 0.f };
    float focus_distance_{ 0.f };

    // gpu data
    ConstBuffer<CAMERA_DATA_BIND> camera_data_cb_;
    CAMERA_DATA_BIND camera_data_;
};
