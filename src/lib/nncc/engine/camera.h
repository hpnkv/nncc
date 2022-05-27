#pragma once

#include <bx/math.h>

#include <nncc/context/hid.h>
#include <nncc/engine/world_math.h>

namespace nncc::engine {

class Camera {
public:
    void Update(float timedelta, const context::MouseState& mouse_state) {
        if (!mouse_down) {
            mouse_last.x = mouse_state.x;
            mouse_last.y = mouse_state.y;
        }

        mouse_down = mouse_state.buttons[static_cast<int>(context::MouseButton::Right)];

        if (mouse_down) {
            mouse_now.x = mouse_state.x;
            mouse_now.y = mouse_state.y;
        }

        mouse_last.z = mouse_now.z;
        mouse_now.z = mouse_state.z;

        const float deltaZ = float(mouse_now.z - mouse_last.z);

        if (mouse_down) {
            const int32_t deltaX = mouse_now.x - mouse_last.x;
            const int32_t deltaY = mouse_now.y - mouse_last.y;

            yaw += mouse_speed * float(deltaX);
            pitch -= mouse_speed * float(deltaY);

            mouse_last.x = mouse_now.x;
            mouse_last.y = mouse_now.y;
        }

        const bx::Vec3 direction = {
                bx::cos(pitch) * bx::sin(yaw),
                bx::sin(pitch),
                bx::cos(pitch) * bx::cos(yaw),
        };

        const bx::Vec3 right = {
                bx::sin(yaw - bx::kPiHalf),
                0.0f,
                bx::cos(yaw - bx::kPiHalf),
        };

        const bx::Vec3 up = bx::cross(right, direction);

        eye_ = bx::mad(direction, deltaZ * timedelta * move_speed, eye_);
        at_ = bx::add(eye_, direction);
        up_ = bx::cross(right, direction);
    }

    Matrix4 GetViewMatrix() {
        Matrix4 result;
        bx::mtxLookAt(result.Data(), eye_, at_, up_);
        return result;
    }

private:
    context::MouseState mouse_last{};
    context::MouseState mouse_now{};

    bx::Vec3 eye_{0., 0., 0.};
    bx::Vec3 up_{0., 0., -1.};
    bx::Vec3 at_{0., 1., 0.};

    float yaw = 0.0f;
    float pitch = 0.01f;

    float mouse_speed = 0.0020f;
    float gamepad_speed = 0.04f;
    float move_speed = 30.0f;

    bool mouse_down = false;
};

}