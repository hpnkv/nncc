#include "loop.h"

#include <imnodes/imnodes.h>

#include <imgui_internal.h>

#include <nncc/context/context.h>
#include <nncc/engine/camera.h>
#include <nncc/gui/gui.h>

namespace nncc::engine {

int LoopThreadFunc(bx::Thread* self, void* args) {
    auto& delegate = *static_cast<ApplicationLoop*>(args);
    auto& context = *context::Context::Get();
    auto& window = context.GetWindow(0);

    if (context.rendering.Init(window.framebuffer_width, window.framebuffer_height) != 0) {
        return 1;
    }
    float font_size = 18;

    if (context.imgui_context == nullptr) {
        imguiCreate(font_size, NULL, window.scale);
        context.imgui_context = ImGui::GetCurrentContext();
        ImGui::GetAllocatorFunctions(&context.imgui_allocators.p_alloc_func, &context.imgui_allocators.p_free_func, &context.imgui_allocators.user_data);
    }

    ImGui::GetStyle().ScaleAllSizes(window.scale);

    ImNodes::CreateContext();

    int result = delegate();

    ImNodes::DestroyContext();
    imguiDestroy();
    context.rendering.Destroy();

    context::GlfwMessage glfw_exit_message;
    glfw_exit_message.type = context::GlfwMessageType::Destroy;
    context.GetMessageQueue().write(glfw_exit_message);

    bgfx::destroy(rendering::Material::default_texture);
    bgfx::shutdown();
    delegate.reset();

    return result;
}

int Run(ApplicationLoop* loop) {
    auto& context = *nncc::context::Context::Get();
    if (!context.InitInMainThread()) {
        return 1;
    }

    auto& window = context.GetWindow(0);
    auto glfw_main_window = context.GetGlfwWindow(0);

    auto thread = context.GetDefaultThread();
    thread->init(&LoopThreadFunc, static_cast<void*>(loop), 0, "main_loop");

    int saved_width, saved_height;
    int xpos, ypos;
    bool fullscreen = false;

    while (!glfwWindowShouldClose(glfw_main_window)) {
        glfwWaitEventsTimeout(1. / 60);

        // TODO: support multiple windows
        int width, height;
        glfwGetWindowSize(glfw_main_window, &width, &height);
        if (width != window.width || height != window.height) {
            auto event = std::make_unique<input::ResizeEvent>();
            event->width = width;
            event->height = height;

            context.input.queue.Push(0, std::move(event));
        }

        nncc::context::GlfwMessage message;
        while (context.GetMessageQueue().read(message)) {
            if (message.type == nncc::context::GlfwMessageType::Destroy) {
                glfwSetWindowShouldClose(glfw_main_window, true);
            } else if (message.type == nncc::context::GlfwMessageType::ToggleFullscreen) {

#if NNCC_PLATFORM_OSX
                ToggleFullscreenCocoa(glfwGetCocoaWindow(glfw_main_window));
                fullscreen = !fullscreen;
#else
                const auto monitor = glfwGetPrimaryMonitor();
                if (!fullscreen) {
                    saved_width = width;
                    saved_height = height;
                    glfwGetWindowPos(glfw_main_window, &xpos, &ypos);
                    const auto video_mode = glfwGetVideoMode(monitor);
                    glfwSetWindowMonitor(glfw_main_window, monitor, 0, 0, 2 * video_mode->width, 2 * video_mode->height, video_mode->refreshRate);
                } else {
                    glfwSetWindowMonitor(glfw_main_window, NULL, xpos, ypos, saved_width, saved_height, 0);
                }
#endif
            }
        }
        bgfx::renderFrame();
    }

    context.Exit();
    while (bgfx::renderFrame() != bgfx::RenderFrame::NoContext) {}
    thread->shutdown();
    return thread->getExitCode();
}
}

//__attribute__((constructor))
//void init(void) {
//    auto& context = nncc::context::Context::Get();
//
//    imguiCreate();
//    ImGui::SetAllocatorFunctions(context.imgui_allocators.p_alloc_func, context.imgui_allocators.p_free_func);
//    ImGui::SetCurrentContext(context.imgui_context);
//}
