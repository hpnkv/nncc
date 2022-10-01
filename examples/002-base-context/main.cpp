#include <nncc/context/context.h>
#include <nncc/engine/timer.h>
#include <nncc/engine/camera.h>
#include <nncc/engine/loop.h>
#include <nncc/gui/imgui_bgfx.h>
#include <nncc/rendering/primitives.h>

using namespace nncc;

entt::entity PlaceUnitPlane(entt::registry* registry, bgfx::ProgramHandle shader) {
    auto mesh_entity = registry->create();

    auto& mesh = registry->emplace<rendering::Mesh>(mesh_entity, rendering::GetPlaneMesh());
    auto& transform = registry->emplace_or_replace<math::Transform>(mesh_entity, math::Transform::Identity());

    auto& material = registry->emplace<rendering::Material>(mesh_entity);
    material.shader = shader;
    material.diffuse_texture = rendering::Material::default_texture;
    material.d_texture_uniform = bgfx::createUniform("diffuseTX", bgfx::UniformType::Sampler, 1);
    material.d_color_uniform = bgfx::createUniform("diffuseCol", bgfx::UniformType::Vec4, 1);

    return mesh_entity;
}

int Loop() {
    // Get references to the context, ENTT registry and window holder
    auto& context = *context::Context::Get();
    auto& window = context.GetWindow(0);

    auto& registry = context.registry;
    const auto& cregistry = context.registry;

    // Create a timer
    engine::Timer timer;

    // Create a camera
    bx::Vec3 eye{1., 0., -10.}, at{0., 1., 0.}, up{0., 0., -1.};
    engine::Camera camera{eye, at, up};

    // Place a mesh close to the origin
    PlaceUnitPlane(&registry, context.rendering.shader_programs_["default_diffuse"]);

    // Frame-by-frame loop
    while (true) {
        if (auto should_exit = context.input.ProcessEvents(); should_exit) {
            bgfx::touch(0);
            bgfx::frame();
            break;
        }
        context.dispatcher.update();

        bgfx::touch(0);

        imguiBeginFrame(context.input.mouse_state.x,
                        context.input.mouse_state.y,
                        context.input.mouse_state.GetImGuiPressedMouseButtons(),
                        context.input.mouse_state.z,
                        uint16_t(window.width),
                        uint16_t(window.height)
        );

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    context.Exit();
                    break;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        imguiEndFrame();

        auto aspect_ratio = static_cast<float>(window.width) / static_cast<float>(window.height);
        camera.SetProjectionMatrix(30.0f, aspect_ratio, 0.01f, 1000.0f);
        camera.Update(timer.Timedelta(), context.input.mouse_state, context.input.key_state, ImGui::MouseOverArea());
        context.rendering.Update(context,
                                 camera.GetViewMatrix(),
                                 camera.GetProjectionMatrix(),
                                 window.width,
                                 window.height);

        // TODO: make this a subsystem's job
        context.input.input_characters.clear();

        bgfx::frame();
        timer.Update();
    }

    return 0;
}

int main() {
    nncc::engine::ApplicationLoop loop;
    loop.connect<&Loop>();

    return nncc::engine::Run(&loop);
}
