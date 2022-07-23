#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <nncc/engine/camera.h>
#include <nncc/engine/loop.h>
#include <nncc/engine/timer.h>
#include <pynncc/torch/tensor_registry.h>
#include <pynncc/torch/shm_communication.h>

#include "node_editor.h"

using namespace nncc;
namespace py = pybind11;

int Loop() {
    using namespace nncc;

    // Get references to the context, ENTT registry and window holder
    auto& context = context::Context::Get();
    auto& window = context.GetWindow(0);

    // Create a thread listening to shared memory handles and a tensor registry
    bx::Thread tensor_update_listener_;
    tensor_update_listener_.init(&python::StartSharedTensorRedisLoop, static_cast<void*>(&context.dispatcher), 0, "tensor_updates");
    python::TensorRegistry tensors;
    tensors.Init(&context.dispatcher);

    // OOP GUI elements (tensor picker in this case)
    nncc::vector<gui::GuiPiece> gui_pieces;
    auto tensor_picker = python::SharedTensorPicker(&tensors);
    gui_pieces.push_back(tensor_picker.GetGuiPiece());

    // Create a timer
    nncc::engine::Timer timer;

    // Create a camera
    bx::Vec3 eye{1., 0., -10.}, at{0., 1., 0.}, up{0., 0., -1.};
    nncc::engine::Camera camera{eye, at, up};

    node_editor::NodeEditorInitialize();

    // Frame-by-frame loop
    while (true) {
        if (auto should_exit = context.input.ProcessEvents(); should_exit) {
            bgfx::touch(0);
            bgfx::frame();
            break;
        }

        context.dispatcher.update();
        tensors.Update();

        bgfx::touch(0);

        imguiBeginFrame(context.input.mouse_state.x,
                        context.input.mouse_state.y,
                        context.input.mouse_state.GetImGuiPressedMouseButtons(),
                        static_cast<int32_t>(context.input.mouse_state.scroll_y),
                        uint16_t(window.width),
                        uint16_t(window.height)
        );
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Tools")) {
                ImGui::MenuItem("Python Interpreter", NULL);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        node_editor::NodeEditorShow(timer.Time());
        for (const auto& gui_piece : gui_pieces) {
            gui_piece.render();
        }
        imguiEndFrame();

        auto aspect_ratio = static_cast<float>(window.width) / static_cast<float>(window.height);
        camera.SetProjectionMatrix(60.0f, aspect_ratio, 0.01f, 1000.0f);
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

    node_editor::NodeEditorShutdown();

    python::StopSharedTensorRedisLoop(python::kRedisQueueName);
    return 0;
}

int main() {
    py::scoped_interpreter guard{};

    nncc::engine::ApplicationLoop loop;
    loop.connect<&Loop>();
    return nncc::engine::Run(&loop);
}