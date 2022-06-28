#include "loop.h"

#include <cpp_redis/cpp_redis>
#include <libshm/libshm.h>

#include <3rdparty/bgfx_imgui/imgui/imgui.h>
#include <nncc/python/shm_communication.h>
#include <nncc/python/tensor_registry.h>

namespace nncc::engine {

int Loop() {
    auto& context = context::Context::Get();
    auto& window = context.GetWindow(0);

    render::PosNormUVVertex::Init();

    engine::Timer timer;
    engine::Camera camera;

    TensorRegistry tensors;
    tensors.Init(&context.dispatcher);

    context.rendering.Init();

    imguiCreate();

    nncc::string selected_name;

    while (true) {
        if (auto should_exit = context.input.ProcessEvents(); should_exit) {
            bgfx::touch(0);
            bgfx::frame();
            break;
        }
        context.dispatcher.update();
        tensors.Update();

        auto& registry = context.registry;
        const auto& cregistry = context.registry;

        uint8_t imgui_pressed_buttons = 0;
        uint8_t current_button_mask = 1;
        for (size_t i = 0; i < 3; ++i) {
            if (context.input.mouse_state.buttons[i]) {
                imgui_pressed_buttons |= current_button_mask;
            }
            current_button_mask <<= 1;
        }

        imguiBeginFrame(context.input.mouse_state.x, context.input.mouse_state.y, imgui_pressed_buttons, context.input.mouse_state.z,
                        uint16_t(window.width),
                        uint16_t(window.height)
        );

        if (!ImGui::MouseOverArea()) {
            camera.Update(timer.Timedelta(), context.input.mouse_state, context.input.key_state);
        }

        bgfx::touch(0);

        ImGui::SetNextWindowPos(ImVec2(50.0f, 50.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320.0f, 800.0f), ImGuiCond_FirstUseEver);

        ImGui::Begin("Example: weight blending");
        ImGui::Text("Shared tensors");
        ImGui::BeginListBox("##label");
        auto named_tensors = cregistry.view<Name, TensorWithPointer>();
        for (auto entity : named_tensors) {
            const auto& [name, tensor_container] = named_tensors.get(entity);
            auto is_selected = selected_name == name.value;
            if (ImGui::Selectable(name.value.c_str(), is_selected)) {
                if (selected_name == name.value) {
                    continue;
                }

                if (!selected_name.empty()) {{
                    auto previously_selected_tensor = tensors.Get(selected_name);
                    if (registry.try_get<render::Material>(previously_selected_tensor)) {
                        auto& mat = registry.get<render::Material>(previously_selected_tensor);
                        mat.diffuse_color = 0xFFFFFFFF;
                    }
                }}
                {
                    auto selected_tensor = tensors.Get(name.value);
                    if (registry.try_get<render::Material>(selected_tensor)) {
                        auto& mat = registry.get<render::Material>(selected_tensor);
                        mat.diffuse_color = 0xDDFFDDFF;
                    }
                }
                selected_name = name.value;
            }
        }
        ImGui::EndListBox();

        if (ImGui::SmallButton(ICON_FA_STOP_CIRCLE)) {
            tensors.Clear();
            selected_name.clear();
        } else if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Remove all shared tensors");
        }

        ImGui::End();
        imguiEndFrame();

        context.rendering.Update(context, camera.GetViewMatrix(), window.width, window.height);

        // TODO: make this a subsystem's job
        context.input.input_characters.clear();

        bgfx::frame();
        timer.Update();
    }

    imguiDestroy();
    context.rendering.Destroy();

    return 0;
}

int RedisListenerThread(bx::Thread* self, void* args) {
    nncc::python::ListenToRedisSharedTensors(&static_cast<nncc::context::Context*>(args)->dispatcher);
    return 0;
}

int MainThreadFunc(bx::Thread* self, void* args) {
    using namespace nncc;

    auto context = static_cast<context::Context*>(args);
    auto& window = context->GetWindow(0);

    bgfx::Init init;
    init.resolution.width = (uint32_t) window.width;
    init.resolution.height = (uint32_t) window.height;
    init.resolution.reset = 0;
    if (!bgfx::init(init)) {
        return 1;
    }

    bx::Thread redis_thread;
    redis_thread.init(&RedisListenerThread, context, 0, "redis");

    int result = Loop();
    context->Destroy();

    bgfx::shutdown();

    redis_thread.shutdown();
    return result;
}

int Run() {
    using namespace nncc::context;

    auto& context = Context::Get();
    if (!context.Init()) {
        return 1;
    }
    bgfx::renderFrame();

    auto& window = context.GetWindow(0);
    auto glfw_main_window = context.GetGlfwWindow(0);

    auto thread = context.GetDefaultThread();
    thread->init(&MainThreadFunc, &context, 0, "rendering");

    while (glfw_main_window != nullptr && !glfwWindowShouldClose(glfw_main_window)) {
        glfwWaitEventsTimeout(1. / 120);

        // TODO: support multiple windows
        int width, height;
        glfwGetWindowSize(glfw_main_window, &width, &height);
        if (width != window.width || height != window.height) {
            auto event = std::make_unique<input::ResizeEvent>();
            event->width = width;
            event->height = height;

            context.input.queue.Push(0, std::move(event));
        }

        GlfwMessage message;
        while (context.GetMessageQueue().read(message)) {
            if (message.type == GlfwMessageType::Destroy) {
                glfwDestroyWindow(glfw_main_window);
                glfw_main_window = nullptr;
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
