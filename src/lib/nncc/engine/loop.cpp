#include "loop.h"

#include <libshm/libshm.h>

#include <3rdparty/bgfx_imgui/imgui/imgui.h>
#include <3rdparty/imguizmo/ImGuizmo.h>
#include <nncc/python/shm_communication.h>
#include <nncc/python/tensor_registry.h>

#define IMGUI_DISABLE_DEBUG_TOOLS

namespace nncc::engine {

bool TensorControlGui(const nncc::string& label, entt::entity tensor_entity, const nncc::string& callback_name) {
    auto& context = context::Context::Get();
    auto& registry = context.registry;

    auto& tensor = *registry.get<TensorWithPointer>(tensor_entity);
    bool updated = false;
    if (tensor.ndimension() <= 1) {
        ImGui::Text("%s", label.c_str());

        if (tensor.dtype() == torch::kFloat32) {
            for (auto i = 0; i < tensor.numel(); ++i) {
                auto* element = tensor.data_ptr<float>() + i;
                if (i > 0) {
                    ImGui::SameLine();
                }
                ImGui::PushItemWidth(30);
                if (ImGui::DragFloat(fmt::format("##{}_dragfloat_{}", label, i).c_str(), element, 0.01f, -10.0f, 10.0f, "%.2f")) {
                    updated = true;
                    break;
                }
            }
        }

        if (tensor.dtype() == torch::kInt32) {
            for (auto i = 0; i < tensor.numel(); ++i) {
                auto* element = tensor.data_ptr<int>() + i;
                if (i > 0) {
                    ImGui::SameLine();
                }
                if (tensor.numel() > 1) {
                    ImGui::PushItemWidth(60);
                    if (ImGui::DragInt(fmt::format("##{}_dragint_{}", label, i).c_str(), element)) {
                        updated = true;
                    }
                } else {
                    if (ImGui::InputInt(fmt::format("##{}_inputint_{}", label, i).c_str(), element)) {
                        updated = true;
                    }
                }
                if (updated) {
                    break;
                }
            }
        }

        if (updated) {
            TensorControlEvent event;
            event.tensor_entity = tensor_entity;
            event.callback_name = callback_name;

            context::Context::Get().dispatcher.enqueue(event);
        }
    }
}

uint8_t GetImGuiPressedMouseButtons(const input::MouseState& mouse_state) {
    uint8_t pressed_buttons = 0;
    uint8_t current_button_mask = 1;
    for (size_t i = 0; i < 3; ++i) {
        if (mouse_state.buttons[i]) {
            pressed_buttons |= current_button_mask;
        }
        current_button_mask <<= 1;
    }
    return pressed_buttons;
}

struct GuiPiece {
    entt::delegate<void()> render;
};

class SharedTensorPicker {
public:
    explicit SharedTensorPicker(TensorRegistry* tensors) : tensors_(*tensors) {}

    void Render() {
        auto& context = context::Context::Get();
        auto& registry = context.registry;
        const auto& cregistry = context.registry;

        ImGui::SetNextWindowPos(ImVec2(50.0f, 50.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320.0f, 800.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Shared tensors")) {
            if (ImGui::BeginListBox("##label")) {
                auto named_tensors = cregistry.view<Name, TensorWithPointer>();
                for (auto entity : named_tensors) {
                    const auto& [name, tensor_container] = named_tensors.get(entity);
                    auto is_selected = selected_name_ == name.value;
                    if (ImGui::Selectable(name.value.c_str(), is_selected)) {
                        if (selected_name_ == name.value) {
                            continue;
                        }

                        if (!selected_name_.empty()) {
                            {
                                auto previously_selected_tensor = tensors_.Get(selected_name_);
                                if (registry.try_get<render::Material>(previously_selected_tensor)) {
                                    auto& mat = registry.get<render::Material>(previously_selected_tensor);
                                    mat.diffuse_color = 0xFFFFFFFF;
                                }
                            }
                        }
                        {
                            auto selected_tensor = tensors_.Get(name.value);
                            if (registry.try_get<render::Material>(selected_tensor)) {
                                auto& mat = registry.get<render::Material>(selected_tensor);
                                mat.diffuse_color = 0xDDFFDDFF;
                            }
                        }
                        selected_name_ = name.value;
                    }
                }
                ImGui::EndListBox();
            }

            if (!tensors_.Contains(selected_name_)) {
                selected_name_.clear();
            } else {
                auto selected_tensor_entity = tensors_.Get(selected_name_);
                if (registry.all_of<TensorWithPointer, Name, TensorControl>(selected_tensor_entity)) {
                    auto& control_callback = registry.get<TensorControl>(selected_tensor_entity).callback_name;
                    auto& name = registry.get<Name>(selected_tensor_entity).value;
                    ImGui::Text("%s", fmt::format("{}: callback name", name).c_str());
                    ImGui::InputText(fmt::format("##{}_callback_name", name).c_str(), &control_callback);
                }
            }

            auto controllable_tensors = registry.view<TensorWithPointer, Name, TensorControl>(entt::exclude<render::Mesh>);
            for (auto&& [entity, tensor, name, control] : controllable_tensors.each()) {
                TensorControlGui(name.value, entity, control.callback_name);
            }

            if (ImGui::SmallButton(ICON_FA_STOP_CIRCLE)) {
                tensors_.Clear();
                selected_name_.clear();
            } else if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Remove all shared tensors");
            }
            ImGui::End();
        }
    }

    GuiPiece GetGuiPiece() {
        GuiPiece tensor_picker_gui;
        tensor_picker_gui.render.connect<&SharedTensorPicker::Render>(this);
        return tensor_picker_gui;
    }

private:
    TensorRegistry& tensors_;
    nncc::string selected_name_;
};

int Loop() {
    auto& context = context::Context::Get();
    auto& window = context.GetWindow(0);

    if (context.rendering.Init(window.width, window.height) != 0) {
        return 1;
    }

    engine::Timer timer;
    engine::Camera camera;

    TensorRegistry tensors;
    tensors.Init(&context.dispatcher);

    cpp_redis::client redis;
    redis.connect();

    nncc::vector<GuiPiece> gui_pieces;

    auto tensor_picker = SharedTensorPicker(&tensors);
    gui_pieces.push_back(tensor_picker.GetGuiPiece());

    imguiCreate();

    nncc::string mask_function_def;
    nncc::string main_mask, second_mask;

    bool show_tools = true;

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

        camera.Update(timer.Timedelta(), context.input.mouse_state, context.input.key_state, ImGui::MouseOverArea());
        bgfx::touch(0);

        imguiBeginFrame(context.input.mouse_state.x,
                        context.input.mouse_state.y,
                        GetImGuiPressedMouseButtons(context.input.mouse_state),
                        context.input.mouse_state.z,
                        uint16_t(window.width),
                        uint16_t(window.height)
        );

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Tools")) {
                ImGui::MenuItem("Blend weights", NULL, &show_tools);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        for (const auto& gui_piece: gui_pieces) {
            gui_piece.render();
        }


        ImGui::SetNextWindowPos(ImVec2(400.0f, 50.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(600.0f, 300.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Blend mask")) {
            auto [gui_width, gui_height] = ImGui::GetWindowSize();
            ImGui::Text("Mask function");
            ImGui::PushItemWidth(std::max(gui_width - 50, 100.0f));
            ImGui::InputTextMultiline("##mask_function", &mask_function_def);
            ImGui::Text("Inputs");
            ImGui::PushItemWidth(std::max(gui_width / 5, 100.f));
            ImGui::InputText("main", &main_mask);
            ImGui::SameLine();
            ImGui::PushItemWidth(std::max(gui_width / 5, 100.f));
            ImGui::InputText("second", &second_mask);
            ImGui::Button("Save");
            ImGui::End();
        }

//        ImGuizmo::BeginFrame();
//        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, window.width, window.height);
//        engine::Matrix4 projection, view = camera.GetViewMatrix();
//        bx::mtxProj(*projection, 30, static_cast<float>(window.width) / static_cast<float>(window.height), 0.01f, 1000.f, bgfx::getCaps()->homogeneousDepth);
//        auto identity = engine::Matrix4::Identity();
//        if (!selected_name.empty()) {
//            auto transform = registry.try_get<Transform>(tensors.Get(selected_name));
//            if (transform != nullptr) {
//                ImGuizmo::Manipulate(*view, *projection, ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, **transform, NULL, NULL);
//            }
//        }

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

int LoopThreadFunc(bx::Thread* self, void* args) {
    using namespace nncc;

    auto& context = context::Context::Get();
    auto& window = context.GetWindow(0);

    bx::Thread redis_thread;
    redis_thread.init(&RedisListenerThread, &context, 0, "redis");

    int result = Loop();
    context.Destroy();

    bgfx::shutdown();

    redis_thread.shutdown();
    return result;
}

int Run() {
    auto& context = nncc::context::Context::Get();
    if (!context.InitInMainThread()) {
        return 1;
    }

    auto& window = context.GetWindow(0);
    auto glfw_main_window = context.GetGlfwWindow(0);

    auto thread = context.GetDefaultThread();
    thread->init(&LoopThreadFunc, &context, 0, "main_loop");

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

        nncc::context::GlfwMessage message;
        while (context.GetMessageQueue().read(message)) {
            if (message.type == nncc::context::GlfwMessageType::Destroy) {
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
