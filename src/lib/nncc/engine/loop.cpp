#include "loop.h"

#include <pybind11/embed.h>
#include <torch/torch.h>
#include <libshm/libshm.h>

#include <3rdparty/bgfx_imgui/imgui/imgui.h>


namespace py = pybind11;


nncc::render::Mesh GetPlaneMesh() {
    nncc::render::Mesh mesh;
    mesh.vertices = {
            {{-1.0f, 1.0f,  0.05f},  {0., 0., 1.},  0., 0.},
            {{1.0f,  1.0f,  0.05f},  {0., 0., 1.},  1., 0.},
            {{-1.0f, -1.0f, 0.05f},  {0., 0., 1.},  0., 1.},
            {{1.0f,  -1.0f, 0.05f},  {0., 0., 1.},  1., 1.},
            {{-1.0f, 1.0f,  -0.05f}, {0., 0., -1.}, 0., 0.},
            {{1.0f,  1.0f,  -0.05f}, {0., 0., -1.}, 1., 0.},
            {{-1.0f, -1.0f, -0.05f}, {0., 0., -1.}, 0., 1.},
            {{1.0f,  -1.0f, -0.05f}, {0., 0., -1.}, 1., 1.},
    };
    mesh.indices = {
            0, 2, 1,
            1, 2, 3,
            1, 3, 2,
            0, 1, 2
    };
    return mesh;
}

//    const auto texture_image = nncc::common::LoadImage("texture.png");
//    const auto texture = nncc::engine::TextureFromImage(texture_image);

//    auto data_ptr = THManagedMapAllocator::makeDataPtr(
//        "/var/folders/5v/0s6hq3gd44z60gsb6nyt01q80000gn/T//torch-shm-dir-NbSNsa/manager.sock",
//        "/torch_38294_2759594541_3",
//        at::ALLOCATOR_MAPPED_SHAREDMEM, 1 * 512 * 512 * 4 * sizeof(float)
//    );
//    auto tensor = torch::from_blob(data_ptr.get(), {1, 512, 512, 4}, torch::TensorOptions().dtype(torch::kF32));

void showDialog(const char* _errorText) {
    char temp[1024];
    bx::snprintf(temp, BX_COUNTOF(temp), "Example: %s", "ImGui support");

    ImGui::SetNextWindowPos(
            ImVec2(10.0f, 50.0f), ImGuiCond_FirstUseEver
    );
    ImGui::SetNextWindowSize(
            ImVec2(300.0f, 210.0f), ImGuiCond_FirstUseEver
    );

    ImGui::Begin(temp);

    ImGui::TextWrapped("%s", "Just testing everything together so far");

    bx::StringView url = "https://apankov.net";
    if (!url.isEmpty()) {
        ImGui::SameLine();
        if (ImGui::SmallButton(ICON_FA_LINK)) {
//            openUrl(url);
        } else if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Documentation: %.*s", url.getLength(), url.getPtr());
        }
    }

    ImGui::Separator();

    if (NULL != _errorText) {
        const int64_t now = bx::getHPCounter();
        const int64_t freq = bx::getHPFrequency();
        const float time = float(now % freq) / float(freq);

        bool blink = time > 0.5f;

        ImGui::PushStyleColor(ImGuiCol_Text, blink
                                             ? ImVec4(1.0, 0.0, 0.0, 1.0)
                                             : ImVec4(1.0, 1.0, 1.0, 1.0)
        );
        ImGui::TextWrapped("%s", _errorText);
        ImGui::Separator();
        ImGui::PopStyleColor();
    }

//    {
//        uint32_t num = entry::getNumApps();
//        const char** items = (const char**) alloca(num * sizeof(void*));
//
//        uint32_t ii = 0;
//        int32_t current = 0;
//        for (entry::AppI* app = entry::getFirstApp(); NULL != app; app = app->getNext()) {
//            if (app == _app) {
//                current = ii;
//            }
//
//            items[ii++] = app->getName();
//        }
//
//        if (1 < num
//            && ImGui::Combo("Example", &current, items, num)) {
//            char command[1024];
//            bx::snprintf(command, BX_COUNTOF(command), "app restart %s", items[current]);
//            cmdExec(command);
//        }
//
//        const bgfx::Caps* caps = bgfx::getCaps();
//        if (0 != (caps->supported & BGFX_CAPS_GRAPHICS_DEBUGGER)) {
//            ImGui::SameLine();
//            ImGui::Text(ICON_FA_SNOWFLAKE_O);
//        }
//
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3.0f, 3.0f));

    if (ImGui::Button(ICON_FA_REPEAT " Restart")) {
//            cmdExec("app restart");
    }

    if (1 < 1) {
        ImGui::SameLine();
        if (ImGui::Button(ICON_KI_PREVIOUS " Prev")) {
//                cmdExec("app restart prev");
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_KI_NEXT " Next")) {
//                cmdExec("app restart next");
        }
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_KI_EXIT " Exit")) {
//            cmdExec("exit");
    }

    ImGui::SameLine();
//        s_showStats ^= ImGui::Button(ICON_FA_BAR_CHART);

    ImGui::PopStyleVar();

    ImGui::End();
}

namespace nncc::engine {

int MainThreadFunc(bx::Thread* self, void* args) {
    auto context = static_cast<nncc::context::Context*>(args);
    auto glfw_window = context->GetGlfwWindow(0);

    bgfx::Init init;

    nncc::render::PosNormUVVertex::Init();

    int32_t width, height;
    glfwGetWindowSize(glfw_window, &width, &height);

    init.resolution.width = (uint32_t) width;
    init.resolution.height = (uint32_t) height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        return 1;
    }

    nncc::engine::Timer timer;
    nncc::engine::Camera camera;
    nncc::render::Renderer renderer{};

    size_t t_width = 512, t_height = 512, t_channels = 4;

    bx::FileReader reader;
    const auto fs = nncc::engine::LoadShader(&reader, "fs_default_diffuse");
    const auto vs = nncc::engine::LoadShader(&reader, "vs_default_diffuse");
    auto program = bgfx::createProgram(vs, fs, true);

    auto texture_uniform = bgfx::createUniform("diffuseTX", bgfx::UniformType::Sampler, 1);
    auto color_uniform = bgfx::createUniform("diffuseCol", bgfx::UniformType::Vec4, 1);
    auto texture = bgfx::createTexture2D(t_width, t_height, false, 0, bgfx::TextureFormat::RGBA32F, 0);

    std::vector<float> mock_texture_data;
    mock_texture_data.resize(t_width * t_height * t_channels);

    auto mesh = GetPlaneMesh();
    nncc::render::Material material{};
    material.diffuse_texture = texture;
    material.diffuse_color = 0xFFFFFFFF;
    material.shader = program;
    material.d_texture_uniform = texture_uniform;
    material.d_color_uniform = color_uniform;

    imguiCreate();

    bool exit = false;
    while (!exit) {
        if (!nncc::engine::ProcessEvents(context)) {
            exit = true;
        }

        uint8_t imgui_pressed_buttons = 0;
        uint8_t current_button_mask = 1;
        for (size_t i = 0; i < 3; ++i) {
            if (context->mouse_state.buttons[i]) {
                imgui_pressed_buttons |= current_button_mask;
            }
            current_button_mask <<= 1;
        }

        imguiBeginFrame(context->mouse_state.x, context->mouse_state.y, imgui_pressed_buttons, context->mouse_state.z, uint16_t(width),
                        uint16_t(height)
        );

        showDialog(nullptr);

        imguiEndFrame();

        auto texture_memory = bgfx::makeRef(mock_texture_data.data(), t_width * t_height * t_channels * 4);
        bgfx::updateTexture2D(texture, 1, 0, 0, 0, t_width, t_height, texture_memory);

        // Time.
        timer.Update();

        if (!ImGui::MouseOverArea()) {
            camera.Update(timer.Timedelta(), context->mouse_state, context->key_state);
        }

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        // Set view 0 default viewport.
        bgfx::setViewRect(0, 0, 0, 1024, 768);

        // This dummy draw call is here to make sure that view 0 is cleared
        // if no other draw calls are submitted to view 0.
        bgfx::touch(0);

        renderer.SetViewMatrix(camera.GetViewMatrix());
        renderer.SetProjectionMatrix(30, static_cast<float>(width) / static_cast<float>(height), 0.01f,
                                     1000.0f);
        renderer.SetViewport({0, 0, static_cast<float>(width), static_cast<float>(height)});
        renderer.Prepare(program);

        auto transform = nncc::engine::Matrix4::Identity();
        bx::mtxRotateY(*transform, timer.Time() * 2.3f);

        renderer.Add(mesh, material, transform);
        renderer.Present();

        bgfx::setDebug(BGFX_DEBUG_STATS);

        // Advance to next frame. Rendering thread will be kicked to
        // process submitted rendering primitives.
        bgfx::frame();
    }

    // finalise
    bgfx::destroy(program);
    bgfx::destroy(texture);

    bgfx::destroy(texture_uniform);
    bgfx::destroy(color_uniform);

    imguiDestroy();
    bgfx::shutdown();
    return 0;
}

int Run() {
    auto& context = nncc::context::Context::Get();
    if (!context.Init()) {
        return 1;
    }
    bgfx::renderFrame();

    auto window = context.GetGlfwWindow(0);

    auto thread = context.GetDefaultThread();
    thread->init(&MainThreadFunc, &context, 0, "render");

    while (window != nullptr && !glfwWindowShouldClose(window)) {
        glfwWaitEventsTimeout(1. / 120);
        while (auto message = context.GetMessageQueue().pop()) {
            if (message->type == nncc::context::GlfwMessageType::Destroy) {
                glfwDestroyWindow(window);
                window = nullptr;
            }
        }
        bgfx::renderFrame();
    }

    std::unique_ptr<nncc::context::Event> event(new nncc::context::ExitEvent);
    event->type = nncc::context::EventType::Exit;
    context.GetEventQueue().Push(0, std::move(event));

    while (bgfx::RenderFrame::NoContext != bgfx::renderFrame()) {}
    thread->shutdown();
    return thread->getExitCode();
}

}

bool nncc::engine::ProcessEvents(nncc::context::Context* context) {
    auto queue = &context->GetEventQueue();
    std::shared_ptr<context::Event> event = queue->Poll();

    if (event == nullptr) {
        return true;
    }

    if (event->type == context::EventType::Exit) {
        return false;

    } else if (event->type == context::EventType::MouseButton) {
        auto btn_event = std::static_pointer_cast<context::MouseEvent>(event);

        context->mouse_state.x = btn_event->x;
        context->mouse_state.y = btn_event->y;
        context->mouse_state.buttons[static_cast<int>(btn_event->button)] = btn_event->down;

    } else if (event->type == context::EventType::MouseMove) {
        auto move_event = std::static_pointer_cast<context::MouseEvent>(event);

        context->mouse_state.x = move_event->x;
        context->mouse_state.y = move_event->y;

    } else if (event->type == context::EventType::Key) {
        auto key_event = std::static_pointer_cast<context::KeyEvent>(event);
        auto key = key_event->key;
        if (key_event->down) {
            context->key_state.pressed_keys.insert(key);
        } else {
            if (context->key_state.pressed_keys.contains(key)) {
                context->key_state.pressed_keys.erase(key);
            }
        }

    } else {
        std::cerr << "unknown event type" << std::endl;
    }

    return true;
}
