#include <pybind11/embed.h>

#include <pynncc/torch/tensor_registry.h>
#include <pynncc/torch/shm_communication.h>
#include <pynncc/compute/python_nodes.h>
#include <pynncc/compute/pytorch_nodes.h>

#include <nncc/compute/algebra_ops.h>
#include <nncc/engine/loop.h>
#include <nncc/engine/timer.h>
#include <nncc/gui/gui.h>
#include <nncc/gui/picking.h>
#include <imgui_internal.h>

using namespace nncc;
namespace py = pybind11;

int Loop() {
    py::scoped_interpreter guard{};

    using namespace nncc;

    // Get references to the context, ENTT registry and window holder
    auto& context = *context::Context::Get();
    auto& window = context.GetWindow(0);

    auto object_picker = gui::ObjectPicker();
    context.subsystems.Register(&object_picker);

    ImGui::SetAllocatorFunctions(context.imgui_allocators.p_alloc_func, context.imgui_allocators.p_free_func);
    ImGui::SetCurrentContext(context.imgui_context);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f * window.scale);

    nncc::python::init();

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
    context.subsystems.Register(&camera, "current_camera");

    compute::ComputeNodeEditor compute_node_editor;
    auto algebra = std::make_shared<compute::ComputeEditorAddMenuItem>(
        "Algebra",
        nncc::vector<std::shared_ptr<compute::ComputeEditorAddMenuItem>>{
            std::make_shared<compute::ComputeEditorAddMenuItem>("Add", &compute::MakeAddOp),
            std::make_shared<compute::ComputeEditorAddMenuItem>("Mul", &compute::MakeMulOp),
            std::make_shared<compute::ComputeEditorAddMenuItem>("Const", &compute::MakeConstOp),
        }
    );
    compute_node_editor.RegisterMenuItem(algebra);
    auto python = std::make_shared<compute::ComputeEditorAddMenuItem>(
        "Python",
        nncc::vector<std::shared_ptr<compute::ComputeEditorAddMenuItem>>{
            std::make_shared<compute::ComputeEditorAddMenuItem>("Python Code", &compute::MakePythonCodeOp),
            std::make_shared<compute::ComputeEditorAddMenuItem>("Get Shared Tensor", &compute::MakeGetSharedTensorOp),
        }
    );
    compute_node_editor.RegisterMenuItem(python);

    // Frame-by-frame loop
    while (true) {
        if (auto should_exit = context.input.ProcessEvents(); should_exit) {
            bgfx::touch(0);
            bgfx::frame();
            break;
        }

        context.dispatcher.update();
//        tensors.Update();

        bgfx::touch(0);

        imguiBeginFrame(context.input.mouse_state.x,
                        context.input.mouse_state.y,
                        context.input.mouse_state.GetImGuiPressedMouseButtons(),
                        static_cast<int32_t>(context.input.mouse_state.scroll_y),
                        uint16_t(window.framebuffer_width),
                        uint16_t(window.framebuffer_height)
        );
        ImGuizmo::BeginFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Python Interpreter", nullptr);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        for (const auto& gui_piece: gui_pieces) {
            gui_piece.render();
        }

        ImGui::SetNextWindowPos(ImVec2(1250.0f * window.scale, 50.0f * window.scale), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320.0f * window.scale, 600.0f * window.scale), ImGuiCond_FirstUseEver);
        ImGui::Begin("Inspector");
        auto selected_nodes = compute_node_editor.GetSelectedNodes();
        if (ImGui::Button("Evaluate graph")) {
            compute_node_editor.GetGraph().Evaluate(&context.registry);
        }
        for (auto node : selected_nodes) {
            if (node->render_context_ui) {
                if (node->render_context_ui(node)) {
                    compute_node_editor.GetGraph().Evaluate(&context.registry);
                }
            }
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(400.0f * window.scale, 50.0f * window.scale), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(800.0f * window.scale, 600.0f * window.scale), ImGuiCond_FirstUseEver);
        compute_node_editor.Update();

        auto viewport = ImGui::GetMainViewport();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        if (ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, ImGui::GetFrameHeight(), window_flags)) {
            if (ImGui::BeginMenuBar()) {
                ImGui::NextColumn();
                nncc::string text = context.log_message;
                auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(text.c_str()).x
                            - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
                if(posX > ImGui::GetCursorPosX())
                    ImGui::SetCursorPosX(posX);
                ImGui::Text("%s", text.c_str());
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        imguiEndFrame();

        auto aspect_ratio = static_cast<float>(window.width) / static_cast<float>(window.height);
        camera.SetProjectionMatrix(60.0f, aspect_ratio, 0.01f, 1000.0f);
        camera.Update(timer.Timedelta(), context.input.mouse_state, context.input.key_state, ImGui::MouseOverArea());
        context.rendering.Update(context,
                                 camera.GetViewMatrix(),
                                 camera.GetProjectionMatrix(),
                                 window.framebuffer_width,
                                 window.framebuffer_height);

        // TODO: make this a subsystem's job
        context.input.input_characters.clear();

        object_picker.Update();
        context.frame_number = bgfx::frame();
        timer.Update();
    }

    object_picker.Destroy();

    python::StopSharedTensorRedisLoop(python::kRedisQueueName);
    return 0;
}

int main() {
    nncc::engine::ApplicationLoop loop;
    loop.connect<&Loop>();
    return nncc::engine::Run(&loop);
}