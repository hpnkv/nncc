#pragma once

#include <bx/math.h>
#include <bgfx/bgfx.h>

#include <nncc/input/hid.h>
#include <nncc/math/types.h>

namespace nncc::engine {

math::Matrix4 DefaultProjectionMatrix();

class Camera {
public:
    Camera(const bx::Vec3& eye, const bx::Vec3& at, const bx::Vec3& up,
           const math::Matrix4& projection_matrix = DefaultProjectionMatrix())
            : eye_(eye), at_(at), up_(up), projection_matrix_(projection_matrix) {}

    void Update(float timedelta,
                const input::MouseState& mouse_state,
                const input::KeyState& key_state,
                bool mouse_over_gui = false);

    math::Matrix4 GetViewMatrix();

    [[nodiscard]] math::Matrix4 GetProjectionMatrix() const {
        return projection_matrix_;
    }

    void SetProjectionMatrix(float field_of_view, float aspect, float near, float far) {
        bx::mtxProj(*projection_matrix_, field_of_view, aspect, near, far, bgfx::getCaps()->homogeneousDepth);
    }

    static math::Matrix4 MakeProjectionMatrix(float field_of_view, float aspect, float near, float far) {
        math::Matrix4 matrix;
        bx::mtxProj(*matrix, field_of_view, aspect, near, far, bgfx::getCaps()->homogeneousDepth);
        return matrix;
    }

private:
    input::MouseState mouse_last_{};
    input::MouseState mouse_now_{};

    bx::Vec3 eye_{2., 0., -10.};
    bx::Vec3 up_{0., 0., -1.};
    bx::Vec3 at_{0., 1., 0.};

    math::Matrix4 projection_matrix_;

    float yaw_ = 0.0f;
    float pitch_ = 0.01f;

    float mouse_speed_ = 0.005f;
    float gamepad_speed_ = 0.04f;
    float move_speed_ = 5.0f;

    bool mouse_down_ = false;
};

}