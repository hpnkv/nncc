#pragma once

#include <bgfx/bgfx.h>

#include <nncc/context/context.h>
#include <nncc/engine/camera.h>
#include <nncc/gui/imgui_bgfx.h>
#include <nncc/rendering/surface.h>
#include <nncc/rendering/renderer.h>

namespace nncc::gui {

bgfx::ProgramHandle LoadProgram(const nncc::string& vs_name, const nncc::string& fs_name);

class ObjectPicker {
public:
    using FloatIds = std::unordered_map<entt::entity, nncc::vector<float>>;
    using UintIds = std::unordered_map<entt::entity, uint32_t>;

    ObjectPicker() {
        id_renderer_ = rendering::Renderer(id_view_id_);

        auto& context = context::Context::Get();

        id_uniform_ = bgfx::createUniform("u_id", bgfx::UniformType::Vec4);
        id_program_ = LoadProgram("vs_picking", "fs_picking");

        auto flags = (
                BGFX_TEXTURE_RT
                | BGFX_SAMPLER_MIN_POINT
                | BGFX_SAMPLER_MAG_POINT
                | BGFX_SAMPLER_MIP_POINT
                | BGFX_SAMPLER_U_CLAMP
                | BGFX_SAMPLER_V_CLAMP
        );
        picking_render_target_ = bgfx::createTexture2D(8, 8, false, 1, bgfx::TextureFormat::RGBA8, flags);
        picking_render_target_depth_ = bgfx::createTexture2D(8, 8, false, 1, bgfx::TextureFormat::D32F, flags);

        auto blit_flags = (
                BGFX_TEXTURE_BLIT_DST
                | BGFX_TEXTURE_READ_BACK
                | BGFX_SAMPLER_MIN_POINT
                | BGFX_SAMPLER_MAG_POINT
                | BGFX_SAMPLER_MIP_POINT
                | BGFX_SAMPLER_U_CLAMP
                | BGFX_SAMPLER_V_CLAMP
        );
        blit_texture_ = bgfx::createTexture2D(8, 8, false, 1, bgfx::TextureFormat::RGBA8, blit_flags);

        bgfx::TextureHandle render_targets[2] = {picking_render_target_, picking_render_target_depth_};
        framebuffer_ = bgfx::createFrameBuffer(2, render_targets, true);

        bgfx::setViewClear(id_view_id_, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
    }

    void RenderUi() {
        auto& context = context::Context::Get();
        const auto& window = context.GetWindow(0);

        ImGui::SetNextWindowPos(
                ImVec2(window.width - window.width / 5.0f - 10.0f, 10.0f), ImGuiCond_FirstUseEver
        );
        ImGui::SetNextWindowSize(
                ImVec2(window.width / 5.0f, window.height / 2.0f), ImGuiCond_FirstUseEver
        );
        ImGui::Begin("Settings");
        ImGui::Image(picking_render_target_, ImVec2(window.width / 5.0f - 16.0f, window.width / 5.0f - 16.0f));
        ImGui::End();
    }

    void Update() {
        auto& context = context::Context::Get();
        const auto& cregistry = context.registry;
        UpdateEntityIds(cregistry);

        const bgfx::Caps* caps = bgfx::getCaps();
        bool blit_supported = 0 != (caps->supported & BGFX_CAPS_TEXTURE_BLIT);

        if (!blit_supported) {
            return;
        }

        const auto& camera = context.subsystems.Get<engine::Camera>("current_camera");
        const auto& window = context.GetWindow(0);

        const auto view_matrix = camera->GetViewMatrix();
        const auto projection_matrix = camera->GetProjectionMatrix();

        bgfx::setViewFrameBuffer(id_view_id_, framebuffer_);

        // Picking pass
        float vp_matrix[16];
        bx::mtxMul(vp_matrix, *view_matrix, *projection_matrix);

        float vp_inverse_matrix[16];
        bx::mtxInverse(vp_inverse_matrix, vp_matrix);

        // Mouse coord in NDC
        const auto& mouse_state = context.input.mouse_state;
        float mouseXNDC = (mouse_state.x / static_cast<float>(window.width)) * 2.0f - 1.0f;
        float mouseYNDC = ((window.height - mouse_state.y) / static_cast<float>(window.height)) * 2.0f - 1.0f;

        const bx::Vec3 pickEye = bx::mulH({mouseXNDC, mouseYNDC, 0.0f}, vp_inverse_matrix);
        const bx::Vec3 pickAt = bx::mulH({mouseXNDC, mouseYNDC, 1.0f}, vp_inverse_matrix);

        // Look at our unprojected point
        math::Matrix4 pickView;
        bx::mtxLookAt(*pickView, pickEye, pickAt);

        // Tight FOV is best for picking
        math::Matrix4 pickProj;
        bx::mtxProj(*pickProj, 1.0f, 1, 0.1f, 100.0f, caps->homogeneousDepth);

        // View rect and transforms for picking pass
        id_renderer_.SetViewMatrix(pickView);
        id_renderer_.SetProjectionMatrix(pickProj);
        id_renderer_.SetViewport({0, 0, 8, 8});
        id_renderer_.Prepare(id_program_);

        auto view = cregistry.view<rendering::Material, rendering::Mesh, math::Transform>();
        for (const auto& [entity, id_uint]: ids_uint_) {
            auto id_float = ids_float_[entity];
            bgfx::setUniform(id_uniform_, id_float.data());

            // submit meshes
            const auto& [_material, mesh, transform] = view.get(entity);
            auto material = _material;
            material.diffuse_texture.idx = bgfx::kInvalidHandle;
            material.diffuse_color = 0xffffffff;
            material.shader = id_program_;

            id_renderer_.Add(mesh, material, transform);
        }

        id_renderer_.Present();

        if (blit_available_frame_ == context.frame_number) {
            blit_available_frame_ = 0;
            InferPickedObjectFromBlit(caps->rendererType == bgfx::RendererType::Direct3D9);
        }

        if (!mouse_down_) {
            if (mouse_state.buttons[static_cast<int>(input::MouseButton::Left)]) {
                mouse_down_ = true;
            }
        }

        if (mouse_down_ && !mouse_state.buttons[static_cast<int>(input::MouseButton::Left)]) {
            mouse_down_ = false;
            if (!blit_available_frame_ &&
                (!ImGui::MouseOverArea() || ImGuizmo::IsUsing())) {
                // Blit and read
                bgfx::blit(blit_view_id_, blit_texture_, 0, 0, picking_render_target_);
                blit_available_frame_ = bgfx::readTexture(blit_texture_, blit_data_);
            }
        }
    }

    void Destroy() {
        bgfx::destroy(id_uniform_);
        bgfx::destroy(id_program_);

        bgfx::destroy(picking_render_target_);
        bgfx::destroy(picking_render_target_depth_);
        bgfx::destroy(blit_texture_);
        bgfx::destroy(framebuffer_);
    }

    void SetPickedObject(entt::entity entity) {
        picked_object_ = entity;
    }

    entt::entity GetPickedObject() const {
        return picked_object_;
    }

private:
    // TODO: query these from the rendering subsystem
    rendering::Renderer id_renderer_;
    uint8_t id_view_id_ = 1;
    uint8_t blit_view_id_ = 2;

    uint32_t current_id_ = 0;
    entt::entity picked_object_ = entt::null;
    FloatIds ids_float_;
    UintIds ids_uint_;

    uint32_t blit_available_frame_ = 0;
    uint8_t blit_data_[8 * 8 * 4]{}; // Read blit into this
    bool mouse_down_ = false;

    bgfx::UniformHandle id_uniform_{};
    bgfx::ProgramHandle id_program_{};

    bgfx::TextureHandle picking_render_target_{};
    bgfx::TextureHandle picking_render_target_depth_{};
    bgfx::TextureHandle blit_texture_{};
    bgfx::FrameBufferHandle framebuffer_{};

    void UpdateEntityIds(const entt::registry& cregistry) {
        FloatIds ids_float;
        UintIds ids_uint;
        auto entities_with_meshes = cregistry.view<rendering::Mesh>();
        for (const auto entity: entities_with_meshes) {
            if (ids_uint_.contains(entity)) {
                ids_float[entity] = ids_float_[entity];
                ids_uint[entity] = ids_uint_[entity];
            } else {
                union {
                    struct {
                        uint8_t r;
                        uint8_t g;
                        uint8_t b;
                        uint8_t a;
                    };
                    uint32_t rgba;
                } new_id{.rgba = ++current_id_};

                ids_uint[entity] = new_id.rgba;
                ids_float[entity].resize(4);
                ids_float[entity][0] = static_cast<float>(new_id.r) / 255.0f;
                ids_float[entity][1] = static_cast<float>(new_id.g) / 255.0f;
                ids_float[entity][2] = static_cast<float>(new_id.b) / 255.0f;
                ids_float[entity][3] = 1.0f;
            }
        }
        ids_float_ = ids_float;
        ids_uint_ = ids_uint;
    }

    void InferPickedObjectFromBlit(bool swap_red_and_blue = false) {
        std::map<uint32_t, uint32_t> id_counts;  // This contains all the IDs found in the buffer
        uint32_t max_count = 0;
        uint32_t max_key = UINT32_MAX;
        for (uint8_t* x = blit_data_; x < blit_data_ + 8 * 8 * 4;) {
            uint8_t rr = *x++;
            uint8_t gg = *x++;
            uint8_t bb = *x++;
            uint8_t aa = *x++;

            if (swap_red_and_blue) {
                // Comes back as BGRA
                uint8_t temp = rr;
                rr = bb;
                bb = temp;
            }

            uint32_t hash_key = rr + (gg << 8) + (bb << 16);
            auto count = ++id_counts[hash_key];
            if (count > max_count) {
                max_count = count;
                max_key = hash_key;
            }
            max_count = max_count > count ? max_count : count;
        }

        if (max_key == 0 || max_count == 0) {
            picked_object_ = entt::null;
            return;
        }

        for (const auto& [entity, id]: ids_uint_) {
            if (id == max_key) {
                picked_object_ = entity;
                break;
            }
        }
    }
};

}