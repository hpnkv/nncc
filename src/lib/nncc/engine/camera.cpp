#include "camera.h"

namespace nncc::engine {

void Camera::Update(float timedelta, const context::MouseState& mouse_state, const context::KeyState& key_state) {
    if (!mouse_down) {
        mouse_last.x = mouse_state.x;
        mouse_last.y = mouse_state.y;
    }

    mouse_down = mouse_state.buttons[static_cast<int>(context::MouseButton::Left)];

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

    if (key_state.pressed_keys.contains(context::Key::KeyW)) {
        eye_ = bx::mad(direction, timedelta * move_speed, eye_);
    }

    if (key_state.pressed_keys.contains(context::Key::KeyS)) {
        eye_ = bx::mad(direction, -timedelta * move_speed, eye_);
    }

    if (key_state.pressed_keys.contains(context::Key::KeyA)) {
        eye_ = bx::mad(right, timedelta * move_speed, eye_);
    }

    if (key_state.pressed_keys.contains(context::Key::KeyD)) {
        eye_ = bx::mad(right, -timedelta * move_speed, eye_);
    }

    eye_ = bx::mad(direction, deltaZ * timedelta * move_speed, eye_);
    at_ = bx::add(eye_, direction);
    up_ = bx::cross(right, direction);
}

Matrix4 Camera::GetViewMatrix() {
    Matrix4 result;
    bx::mtxLookAt(result.Data(), eye_, at_, up_);
    return result;
}
}