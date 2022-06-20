#include "loop.h"

#include <cpp_redis/cpp_redis>
#include <libshm/libshm.h>

#include <3rdparty/bgfx_imgui/imgui/imgui.h>
#include <nncc/python/shm_communication.h>
#include <nncc/python/tensor_plane.h>

namespace nncc::engine {

int Loop(context::Context* context) {
    auto& window = context->GetWindow(0);

    render::PosNormUVVertex::Init();

    engine::Timer timer;
    engine::Camera camera;
    render::Renderer renderer{};

    bx::FileReader reader;
    const auto fs = nncc::engine::LoadShader(&reader, "fs_default_diffuse");
    const auto vs = nncc::engine::LoadShader(&reader, "vs_default_diffuse");
    auto program = bgfx::createProgram(vs, fs, true);
    context->shader_programs["default_diffuse"] = program;

    imguiCreate();

    std::unordered_map<nncc::string, TensorPlane> tensor_planes;
    context->user_storages["tensor_planes"] = static_cast<void*>(&tensor_planes);

    nncc::string selected_tensor;

    while (true) {
        if (!engine::ProcessEvents(context)) {
            bgfx::touch(0);
            bgfx::frame();
            break;
        }

        uint8_t imgui_pressed_buttons = 0;
        uint8_t current_button_mask = 1;
        for (size_t i = 0; i < 3; ++i) {
            if (context->mouse_state.buttons[i]) {
                imgui_pressed_buttons |= current_button_mask;
            }
            current_button_mask <<= 1;
        }

        if (!ImGui::MouseOverArea()) {
            camera.Update(timer.Timedelta(), context->mouse_state, context->key_state);
        }

        for (auto& [key, tensor_plane] : tensor_planes) {
            tensor_plane.Update();
        }

        bgfx::touch(0);

        imguiBeginFrame(context->mouse_state.x, context->mouse_state.y, imgui_pressed_buttons, context->mouse_state.z,
                        uint16_t(window.width),
                        uint16_t(window.height)
        );

        ImGui::SetNextWindowPos(ImVec2(20.0f, 50.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(240.0f, 300.0f), ImGuiCond_FirstUseEver);

        ImGui::Begin("Example: PyTorch shared memory tensors");

        ImGui::Text("Shared tensors");
        ImGui::BeginListBox("##label");
        for (const auto& [name, tensor_plane] : tensor_planes) {
            auto selected = name == selected_tensor;
            if (ImGui::Selectable(name.c_str(), selected)) {
                selected_tensor = name;
            }
        }
        ImGui::EndListBox();

        if (ImGui::SmallButton(ICON_FA_LINK)) {
//            openUrl(url);
        } else if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Documentation: https://apankov.net");
        }

        ImGui::End();
        imguiEndFrame();

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, window.width, window.height);

        renderer.SetViewMatrix(camera.GetViewMatrix());
        renderer.SetProjectionMatrix(30, static_cast<float>(window.width) / static_cast<float>(window.height), 0.01f,
                                     1000.0f);
        renderer.SetViewport({0, 0, static_cast<float>(window.width), static_cast<float>(window.height)});

        for (const auto& [key, tensor_plane] : tensor_planes) {
            tensor_plane.Render(&renderer, timer.Time());
        }

        renderer.Present();

        // TODO: make this a subsystem's job
        context->input_characters.clear();

        bgfx::setDebug(BGFX_DEBUG_STATS);

        bgfx::frame();

        // Time.
        timer.Update();
    }

    imguiDestroy();

    bgfx::destroy(nncc::render::Material::default_texture);

    return 0;
}

int RedisListenerThread(bx::Thread* self, void* args) {
    nncc::python::ListenToRedisSharedTensors(static_cast<nncc::context::Context*>(args));
    return 0;
}

int MainThreadFunc(bx::Thread* self, void* args) {
    using namespace nncc;

    auto context = static_cast<context::Context*>(args);
    auto& window = context->GetWindow(0);

    bgfx::Init init;
    init.resolution.width = (uint32_t) window.width;
    init.resolution.height = (uint32_t) window.height;
    init.resolution.reset = BGFX_RESET_VSYNC;
    if (!bgfx::init(init)) {
        return 1;
    }

    bx::Thread redis_thread;
    redis_thread.init(&RedisListenerThread, context, 0, "redis");

    int result = Loop(context);
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
    thread->init(&MainThreadFunc, &context, 0, "render");

    while (glfw_main_window != nullptr && !glfwWindowShouldClose(glfw_main_window)) {
        glfwWaitEventsTimeout(1. / 120);

        // TODO: support multiple windows
        int width, height;
        glfwGetWindowSize(glfw_main_window, &width, &height);
        if (width != window.width || height != window.height) {
            auto event = std::make_unique<ResizeEvent>();
            event->width = width;
            event->height = height;

            context.GetEventQueue().Push(0, std::move(event));
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

bool nncc::engine::ProcessEvents(nncc::context::Context* context) {
    using namespace nncc::context;

    auto queue = &context->GetEventQueue();
    std::shared_ptr<Event> event = queue->Poll();

    while (event != nullptr) {
        if (event->type == EventType::Exit) {
            return false;

        } else if (event->type == EventType::Resize) {
            auto resize_event = std::dynamic_pointer_cast<ResizeEvent>(event);
            context->SetWindowResolution(event->window_idx, resize_event->width, resize_event->height);
            bgfx::reset(resize_event->width, resize_event->height, BGFX_RESET_VSYNC);
            bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);

        } else if (event->type == EventType::MouseButton) {
            auto btn_event = std::dynamic_pointer_cast<MouseEvent>(event);

            context->mouse_state.x = btn_event->x;
            context->mouse_state.y = btn_event->y;
            context->mouse_state.buttons[static_cast<int>(btn_event->button)] = btn_event->down;

        } else if (event->type == EventType::MouseMove) {
            auto move_event = std::dynamic_pointer_cast<MouseEvent>(event);

            context->mouse_state.x = move_event->x;
            context->mouse_state.y = move_event->y;

        } else if (event->type == EventType::Key) {
            auto key_event = std::dynamic_pointer_cast<KeyEvent>(event);
            context->key_state.modifiers = key_event->modifiers;

            auto key = key_event->key;
            if (key_event->down) {
                context->key_state.pressed_keys.insert(key);
            } else if (context->key_state.pressed_keys.contains(key)) {
                context->key_state.pressed_keys.erase(key);
            }

        } else if (event->type == EventType::Char) {
            auto char_event = std::dynamic_pointer_cast<CharEvent>(event);
            context->input_characters.push_back(char_event->codepoint);

        } else if (event->type == EventType::SharedTensor) {
            std::cout << "tensor event" << std::endl;
            auto tensor_event = std::dynamic_pointer_cast<SharedTensorEvent>(event);
            auto plane = TensorPlane(
                    tensor_event->manager_handle,
                    tensor_event->filename,
                    tensor_event->dtype,
                    tensor_event->dims
            );

            auto planes = static_cast<std::unordered_map<nncc::string, TensorPlane>*>(context->user_storages["tensor_planes"]);

            auto identity = engine::Matrix4::Identity();

            Matrix4 x_translation;
            bx::mtxTranslate(*x_translation, 2.1f * planes->size(), 0., 0.);
            Matrix4 position_2;
            bx::mtxMul(*position_2, *identity, *x_translation);
            plane.SetTransform(position_2);

            planes->emplace(tensor_event->name, std::move(plane));

        } else {
            std::cerr << "unknown event type " << static_cast<int>(event->type) << std::endl;
        }

        event = queue->Poll();
    }

    return true;
}
