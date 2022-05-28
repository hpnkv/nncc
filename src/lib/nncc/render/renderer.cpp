#include "renderer.h"

namespace nncc::render {

void Renderer::Add(const Mesh& mesh, const Material& material, const engine::Matrix4& transform) {
    batch_renderer_ctx_.Add(view_id_, mesh, material, transform, 0);
}

void Renderer::Present() {
    batch_renderer_ctx_.Flush();
    ++view_id_;

    view_matrix_.reset();
    projection_matrix_.reset();
    viewport_.reset();
}

void Renderer::SetViewMatrix(const engine::Matrix4& matrix) {
    view_matrix_ = matrix;
}

void Renderer::SetViewMatrix(const bx::Vec3& eye, const bx::Vec3& at, const bx::Vec3& up) {
    engine::Matrix4 matrix;
    bx::mtxLookAt(*matrix, eye, at, up);
    view_matrix_ = matrix;
}

void Renderer::SetProjectionMatrix(float field_of_view, float aspect, float near, float far) {
    engine::Matrix4 matrix;
    bx::mtxProj(*matrix, field_of_view, aspect, near, far, bgfx::getCaps()->homogeneousDepth);
    projection_matrix_ = matrix;
}

void Renderer::SetViewport(const Viewport& viewport) {
    viewport_ = viewport;
}

void Renderer::Prepare(bgfx::ProgramHandle program) {
    bgfx::setViewRect(view_id_, uint16_t(viewport_->x), uint16_t(viewport_->y), uint16_t(viewport_->width),
                      uint16_t(viewport_->height));

    Init();

    if (!viewport_ || !view_matrix_ || !projection_matrix_) {
        throw std::runtime_error("Init view before rendering.");
    }

    bx::mtxMul(*cached_vp_matrix_, **view_matrix_, **projection_matrix_);
    bx::mtxInverse(*cached_vp_matrix_inv_, *cached_vp_matrix_);

    bgfx::setViewTransform(view_id_, **view_matrix_, **projection_matrix_);
    bgfx::setViewMode(view_id_, bgfx::ViewMode::Default);

    batch_renderer_ctx_.Prepare(vertex_layout_);
    batch_renderer_ctx_.Init(program);
}

void Renderer::Init() {
    if (initialised_) {
        return;
    }

    vertex_layout_ = &PosNormUVVertex::layout;
    initialised_ = true;
}

}