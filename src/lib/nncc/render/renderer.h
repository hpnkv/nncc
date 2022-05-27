#pragma once

#include <array>
#include <vector>
#include <optional>

#include <cmath>
#include <bgfx/bgfx.h>
#include <bx/math.h>

#include <nncc/render/surface.h>
#include <nncc/render/batch_renderer.h>

namespace nncc::render {

class Renderer {
public:
    void Add(const Mesh& mesh, const Material& material, const engine::Matrix4& transform);

    void Present();

    void SetViewMatrix(const bx::Vec3& eye, const bx::Vec3& at, const bx::Vec3& up = {0, 1, 0});

    void SetProjectionMatrix(float field_of_view, float aspect, float near, float far);

    void SetViewport(const Viewport& viewport);

    void Prepare(bgfx::ProgramHandle program = BGFX_INVALID_HANDLE);

private:
    void Init();

    bgfx::VertexLayout* vertex_layout_ = nullptr;
    BatchRenderer batch_renderer_ctx_;

    std::optional<Viewport> viewport_;
    std::optional<engine::Matrix4> view_matrix_;
    std::optional<engine::Matrix4> projection_matrix_;

    engine::Matrix4 cached_vp_matrix_;
    engine::Matrix4 cached_vp_matrix_inv_;

    uint8_t view_id_;
    bool initialised_;
};

}

