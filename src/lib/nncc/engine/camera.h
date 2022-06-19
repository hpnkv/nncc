#pragma once

#include <bx/math.h>

#include <nncc/context/hid.h>
#include <nncc/engine/world_math.h>

namespace nncc::engine {

class Camera {
public:
    void Update(float timedelta, const context::MouseState& mouse_state, const context::KeyState& key_state);

    Matrix4 GetViewMatrix();

private:
    context::MouseState mouse_last{};
    context::MouseState mouse_now{};

    bx::Vec3 eye_{1., 0., -10.};
    bx::Vec3 up_{0., 0., -1.};
    bx::Vec3 at_{0., 1., 0.};

    float yaw = 0.0f;
    float pitch = 0.01f;

    float mouse_speed = 0.005f;
    float gamepad_speed = 0.04f;
    float move_speed = 5.0f;

    bool mouse_down = false;
};

}