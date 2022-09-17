#pragma once

#include <array>
#include <vector>
#include <optional>

#include <cmath>
#include <bgfx/bgfx.h>
#include <bx/math.h>

#include <nncc/rendering/surface.h>
#include <nncc/rendering/batch_renderer.h>

namespace nncc::rendering {

class Renderer {
public:
    explicit Renderer(uint8_t view_id = 0) : view_id_(view_id) {}

    void Add(const Mesh& mesh, const Material& material, const math::Transform& transform);

    void Present();

    void SetViewMatrix(const math::Matrix4& matrix);

    void SetViewMatrix(const bx::Vec3& eye, const bx::Vec3& at, const bx::Vec3& up = {0, 1, 0});

    void SetProjectionMatrix(const math::Matrix4& matrix) {
        projection_matrix_ = matrix;
    }

    void SetProjectionMatrix(float field_of_view, float aspect, float near, float far);

    void SetViewport(const Viewport& viewport);

    void Prepare(bgfx::ProgramHandle program = BGFX_INVALID_HANDLE);

private:
    void Init();

    bgfx::VertexLayout* vertex_layout_ = nullptr;
    BatchRenderer batch_renderer_ctx_ {};

    std::optional<Viewport> viewport_;
    std::optional<math::Matrix4> view_matrix_;
    std::optional<math::Matrix4> projection_matrix_;

    math::Matrix4 cached_vp_matrix_;
    math::Matrix4 cached_vp_matrix_inv_;

    uint8_t view_id_ = 0;
    bool initialised_ = false;
};

}

