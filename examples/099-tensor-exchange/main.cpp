#include <boost/graph/graphviz.hpp>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <nncc/engine/camera.h>
#include <nncc/engine/loop.h>
#include <nncc/engine/timer.h>
#include <nncc/gui/nodes/graph.h>
#include <pynncc/torch/tensor_registry.h>
#include <pynncc/torch/shm_communication.h>

#include "node_editor.h"

using namespace nncc;
namespace py = pybind11;

//namespace nodes {
//
//Result EvaluateTensorInputNode(Node* node, entt::registry* registry) {
//    Result result;
//    if (!node->settings_by_name.contains("name")) {
//        result.message = "node does not contain a `name` setting";
//        return result;
//    }
//
//    auto& name_attribute = node->settings_by_name.at("name");
//    if (name_attribute.type != AttributeType::String) {
//        result.message = "`name` setting is not a string.";
//        return result;
//    }
//
//    if (name_attribute.value == nullptr) {
//        result.message = "`name` setting is empty.";
//        return result;
//    }
//
//    const auto& name = *static_cast<nncc::string*>(name_attribute.value);
//    auto tensors = static_cast<python::TensorRegistry*>(node->settings_by_name.at("tensor_registry").value);
//
//    auto entity = tensors->Get(name);
//    auto& out_attribute = node->outputs_by_name.at("tensor");
//    out_attribute.entity = entity;
//
//    result.code = 0;
//    result.message = "";
//
//    return result;
//}
//
//Node MakeTensorInputNode(python::TensorRegistry* tensors) {
//    Node node;
//    node.AddSetting({"name", AttributeType::String});
//    node.AddSetting({.name = "tensor_registry", .type = AttributeType::UserDefined, .value = static_cast<void*>(tensors)});
//    node.AddOutput({"tensor", AttributeType::Tensor});
//
//    node.evaluate.connect<&EvaluateTensorInputNode>();
//
//    return node;
//}
//
//}



int Loop() {
    using namespace nncc;

    // Get references to the context, ENTT registry and window holder
    auto& context = context::Context::Get();
    auto& window = context.GetWindow(0);

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

    nodes::ComputeGraph graph;
    nodes::ComputeNode independent_node, dependent_node;
    dependent_node.AddInput({.name = "input", .type = nodes::AttributeType::String});
    independent_node.AddOutput({.name = "output", .type = nodes::AttributeType::String});

    auto dependent = boost::add_vertex(dependent_node, graph);
    auto independent = boost::add_vertex(independent_node, graph);
    boost::add_edge(independent, dependent, {"output", "input"}, graph);
    boost::write_graphviz(std::cout, graph, [&](auto& out, auto v) {
                              out << "[label=\"" << graph[v].id << "\"]";
                          },
                          [&](auto& out, auto e) {
                              out << "[label=\"" << graph[e].from_output << "->" << graph[e].to_input << "\"]";
                          });
    std::cout << std::flush;

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
                ImGui::MenuItem("Python Interpreter", nullptr);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        node_editor::NodeEditorShow(timer.Time());
        for (const auto& gui_piece: gui_pieces) {
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