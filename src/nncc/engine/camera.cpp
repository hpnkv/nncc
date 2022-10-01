#include "camera.h"

namespace nncc::engine {

void Camera::Update(float timedelta,
                    const input::MouseState& mouse_state,
                    const input::KeyState& key_state,
                    bool mouse_over_gui) {
    const bx::Vec3 direction = {
            bx::cos(pitch_) * bx::sin(yaw_),
            bx::sin(pitch_),
            bx::cos(pitch_) * bx::cos(yaw_),
    };

    const bx::Vec3 right = {
            bx::sin(yaw_ - bx::kPiHalf),
            0.0f,
            bx::cos(yaw_ - bx::kPiHalf),
    };

    if (!mouse_over_gui) {
        if (key_state.pressed_keys.contains(input::Key::KeyW)) {
            eye_ = bx::mad(direction, timedelta * move_speed_, eye_);
        }

        if (key_state.pressed_keys.contains(input::Key::KeyS)) {
            eye_ = bx::mad(direction, -timedelta * move_speed_, eye_);
        }

        if (key_state.pressed_keys.contains(input::Key::KeyA)) {
            eye_ = bx::mad(right, timedelta * move_speed_, eye_);
        }

        if (key_state.pressed_keys.contains(input::Key::KeyD)) {
            eye_ = bx::mad(right, -timedelta * move_speed_, eye_);
        }

        if (key_state.pressed_keys.contains(input::Key::KeyE)) {
            eye_ = bx::mad(up_, timedelta * move_speed_, eye_);
        }

        if (key_state.pressed_keys.contains(input::Key::KeyQ)) {
            eye_ = bx::mad(up_, -timedelta * move_speed_, eye_);
        }

        if (!mouse_down_) {
            mouse_last_.x = mouse_state.x;
            mouse_last_.y = mouse_state.y;
        }

        mouse_down_ = mouse_state.buttons[static_cast<int>(input::MouseButton::Right)];

        if (mouse_down_) {
            mouse_now_.x = mouse_state.x;
            mouse_now_.y = mouse_state.y;
        }

        mouse_last_.scroll_y = mouse_now_.scroll_y;
        mouse_now_.scroll_y = mouse_state.scroll_y;

        const auto delta_scroll = float(mouse_now_.scroll_y - mouse_last_.scroll_y);

        if (mouse_down_) {
            const int32_t deltaX = mouse_now_.x - mouse_last_.x;
            const int32_t deltaY = mouse_now_.y - mouse_last_.y;

            yaw_ += mouse_speed_ * float(deltaX);
            pitch_ -= mouse_speed_ * float(deltaY);

            mouse_last_.x = mouse_now_.x;
            mouse_last_.y = mouse_now_.y;
        }
        eye_ = bx::mad(direction, delta_scroll * timedelta * move_speed_, eye_);
    }

    at_ = bx::add(eye_, direction);
    up_ = bx::cross(right, direction);
}

math::Matrix4 Camera::GetViewMatrix() const {
    math::Matrix4 result;
    bx::mtxLookAt(result.Data(), eye_, at_, up_);
    return result;
}

Camera::Camera(const bx::Vec3& eye, const bx::Vec3& at, const bx::Vec3& up, const math::Matrix4& projection_matrix)
    : eye_(eye), at_(at), up_(up), projection_matrix_(projection_matrix) {}

math::Matrix4 Camera::GetProjectionMatrix() const {
    return projection_matrix_;
}

void Camera::SetProjectionMatrix(float field_of_view, float aspect, float near, float far) {
    bx::mtxProj(*projection_matrix_, field_of_view, aspect, near, far, bgfx::getCaps()->homogeneousDepth);
}

math::Matrix4 Camera::MakeProjectionMatrix(float field_of_view, float aspect, float near, float far) {
    math::Matrix4 matrix;
    bx::mtxProj(*matrix, field_of_view, aspect, near, far, bgfx::getCaps()->homogeneousDepth);
    return matrix;
}

math::Matrix4 DefaultProjectionMatrix() {
    math::Matrix4 matrix;
    bx::mtxProj(*matrix, 30.0f, 1.0f, 0.01f, 1000.0f, bgfx::getCaps()->homogeneousDepth);
    return matrix;
}
}