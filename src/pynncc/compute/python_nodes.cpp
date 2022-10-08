#include "python_nodes.h"

#include <nncc/gui/imgui_folly.h>
#include <pynncc/torch/tensor_registry.h>

namespace nncc::compute {

auto PythonCodeOpEvaluateFn(ComputeNode* node, entt::registry* registry) {
    auto& state = *node->StateAs<PythonCodeOpState>();
    auto& context = *context::Context::Get();

    auto result = std::string(py::str(py::eval("generate(None)")));
    context.log_message = result;

    return Result{0, ""};
}

auto PythonCodeOpRenderFn(ComputeNode* node) {
    auto& state = *node->StateAs<PythonCodeOpState>();

    ImGui::InputTextMultiline("##pythoncode", &state.code);
    if (ImGui::Button("Update")) {
        py::exec(state.code.c_str());
        return true;
    }

    return false;
}

ComputeNode MakePythonCodeOp(const void* _) {
    ComputeNode node;

    node.name = "Python code";
    node.type = "PythonCode";

    node.AddOutput(Attribute("value", AttributeType::UserDefined));

    node.evaluate.connect<&PythonCodeOpEvaluateFn>();
    node.render_context_ui.connect<&PythonCodeOpRenderFn>();

    node.state = std::make_shared<PythonCodeOpState>();

    return node;
}


auto GetSharedTensorOpEvaluateFn(ComputeNode* node, entt::registry* registry) {
    auto& state = *node->StateAs<GetSharedTensorOpState>();
    auto& context = *context::Context::Get();

    auto tensor_registry = context.subsystems.Get<nncc::python::TensorRegistry>();
    if (tensor_registry == nullptr) {
        return Result{1, "Tensor registry is not initialised"};
    }

    auto tensor_entity = tensor_registry->Get(state.name);
    state.entity = tensor_entity;

    node->outputs_by_name.at("tensor").entity = state.entity;
    return Result{0, ""};
}

auto GetSharedTensorOpRenderFn(ComputeNode* node) {
    auto& state = *node->StateAs<GetSharedTensorOpState>();

    ImGui::InputText("Name", &state.name);

    if (state.entity != entt::null) {
        ImGui::Button("Contains a tensor");
    } else {
        ImGui::Button("Is empty");
    }

    return false;
}


ComputeNode MakeGetSharedTensorOp(const void* _) {
    ComputeNode node;

    node.name = "Get Shared Tensor";
    node.type = "GetSharedTensor";

    node.AddOutput(Attribute("tensor", AttributeType::UserDefined));

    node.evaluate.connect<&GetSharedTensorOpEvaluateFn>();
    node.render_context_ui.connect<&GetSharedTensorOpRenderFn>();

    node.state = std::make_shared<GetSharedTensorOpState>();

    return node;
}


}