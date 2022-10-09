#include <pynncc/compute/pytorch_nodes.h>

namespace nncc::compute {

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