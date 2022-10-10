#include <nncc/gui/imgui_demo.h>

#include <nncc/context/context.h>
#include <nncc/gui/imgui_bgfx.h>
#include <nncc/engine/loop.h>
#include <nncc/engine/camera.h>
#include <nncc/engine/timer.h>


int Loop() {
    // Get references to the context, ENTT registry and window holder
    auto& context = *nncc::context::Context::Get();
    auto& window = context.GetWindow(0);

    ImGui::SetAllocatorFunctions(context.imgui_allocators.p_alloc_func, context.imgui_allocators.p_free_func);
    ImGui::SetCurrentContext(context.imgui_context);

    ImGui::SetAllocatorFunctions(context.imgui_allocators.p_alloc_func, context.imgui_allocators.p_free_func);
    ImGui::SetCurrentContext(context.imgui_context);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f * window.scale);

    // Create a timer
    nncc::engine::Timer timer;

    // Create a camera
    bx::Vec3 eye{1., 0., -10.}, at{0., 1., 0.}, up{0., 0., -1.};
    nncc::engine::Camera camera{eye, at, up};

    // Frame-by-frame loop
    while (true) {
        if (auto should_exit = context.input.ProcessEvents(); should_exit) {
            bgfx::touch(0);
            bgfx::frame();
            break;
        }

        bgfx::touch(0);

        imguiBeginFrame(context.input.mouse_state.x,
                        context.input.mouse_state.y,
                        context.input.mouse_state.GetImGuiPressedMouseButtons(),
                        context.input.mouse_state.scroll_y,
                        uint16_t(window.framebuffer_width),
                        uint16_t(window.framebuffer_height)
        );
        ImGui::ShowDemoWindow();
        imguiEndFrame();

        auto aspect_ratio = static_cast<float>(window.width) / static_cast<float>(window.height);
        camera.SetProjectionMatrix(30.0f, aspect_ratio, 0.01f, 1000.0f);
        camera.Update(timer.Timedelta(), context.input.mouse_state, context.input.key_state, ImGui::MouseOverArea());
        context.rendering.Update(context,
                                 camera.GetViewMatrix(),
                                 camera.GetProjectionMatrix(),
                                 window.framebuffer_width,
                                 window.framebuffer_height);

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