#include <boost/graph/graphviz.hpp>
#include <pybind11/embed.h>

#include <pynncc/torch/tensor_registry.h>
#include <pynncc/torch/shm_communication.h>

#include <nncc/engine/camera.h>
#include <nncc/engine/loop.h>
#include <nncc/engine/timer.h>
#include <nncc/gui/nodes/graph.h>

using namespace nncc;
namespace py = pybind11;

int Loop() {
    using namespace nncc;

    // Get references to the context, ENTT registry and window holder
    auto& context = context::Context::Get();
    auto& window = context.GetWindow(0);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);

    // Create a thread listening to shared memory handles and a tensor registry
    bx::Thread tensor_update_listener_;
    tensor_update_listener_.init(&python::StartSharedTensorRedisLoop, static_cast<void*>(&context.dispatcher), 0,
                                 "tensor_updates");
    python::TensorRegistry tensors;
    tensors.Init(&context.dispatcher);

    // OOP GUI elements (tensor picker in this case)
    nncc::vector<gui::GuiPiece> gui_pieces;
    auto tensor_picker = python::SharedTensorPicker(&tensors);
    gui_pieces.push_back(tensor_picker.GetGuiPiece());

    // Create a timer
    engine::Timer timer;

    // Create a camera
    bx::Vec3 eye{1., 0., -10.}, at{0., 1., 0.}, up{0., 0., -1.};
    engine::Camera camera{eye, at, up};

    nodes::ComputeNodeEditor compute_node_editor;

//    boost::write_graphviz(std::cout, graph, [&](auto& out, auto v) {
//                              out << "[label=\"" << graph[v].id << "\"]";
//                          },
//                          [&](auto& out, auto e) {
//                              out << "[label=\"" << graph[e].from_output << "->" << graph[e].to_input << "\"]";
//                          });

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
                ImGui::MenuItem("Python Interpreter", nullptr);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        for (const auto& gui_piece: gui_pieces) {
            gui_piece.render();
        }

        ImGui::SetNextWindowPos(ImVec2(1250.0f, 50.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320.0f, 600.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Compute nodes context");
        auto selected_nodes = compute_node_editor.GetSelectedNodes();
        if (ImGui::Button("Evaluate graph")) {
            compute_node_editor.GetGraph().Evaluate(&context.registry);
        }
        if (selected_nodes.size() == 1) {
            if (selected_nodes[0]->render_context_ui) {
                selected_nodes[0]->render_context_ui(selected_nodes[0]);
            }
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(400.0f, 50.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(800.0f, 600.0f), ImGuiCond_FirstUseEver);
        compute_node_editor.Update();

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

    python::StopSharedTensorRedisLoop(python::kRedisQueueName);
    return 0;
}

int main() {
    py::scoped_interpreter guard{};

    nncc::engine::ApplicationLoop loop;
    loop.connect<&Loop>();
    return nncc::engine::Run(&loop);
}