#include "graph.h"

namespace nncc::nodes {

int Attribute::id_counter = 0;
int ComputeNode::id_counter = 0;

auto ConstOpEvaluateFn(ComputeNode* node, entt::registry* registry) {
    node->outputs_by_name.at("value").value = node->settings_by_name.at("value").value;
    return Result{0, ""};
}

auto ConstOpRenderFn(ComputeNode* node) {
    ImGui::InputFloat("Value", &node->settings_by_name.at("value").value, 0.1f, 1.0f, "%0.2f");
}

ComputeNode MakeConstOp(const string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Const";

    node.AddSetting(Attribute("value", AttributeType::UserDefined));
    node.AddOutput(Attribute("value", AttributeType::UserDefined));

    node.evaluate.connect<&ConstOpEvaluateFn>();
    node.render_context_ui.connect<&ConstOpRenderFn>();

    return node;
}

auto AddOpEvaluateFn(ComputeNode* node, entt::registry* registry) {
    auto a = node->inputs_by_name.at("a").value;
    auto b = node->inputs_by_name.at("b").value;

    node->outputs_by_name.at("result").value = a + b;

    return Result{0, ""};
}

auto AddOpRenderFn(ComputeNode* node) {
    ImGui::InputFloat("result", &node->outputs_by_name.at("result").value, 0.0f, 0.0f, "%0.2f", ImGuiInputTextFlags_ReadOnly);
}

ComputeNode MakeAddOp(const string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Add";

    node.AddInput(Attribute("a", AttributeType::UserDefined));
    node.AddInput(Attribute("b", AttributeType::UserDefined));

    node.AddOutput(Attribute("result", AttributeType::UserDefined));

    node.evaluate.connect<&AddOpEvaluateFn>();
    node.render_context_ui.connect<&AddOpRenderFn>();

    return node;
}

auto MulOpEvaluateFn(ComputeNode* node, entt::registry* registry) {
    auto a = node->inputs_by_name.at("a").value;
    auto b = node->inputs_by_name.at("b").value;

    node->outputs_by_name.at("result").value = a * b;

    return Result{0, ""};
}

auto MulOpRenderFn(ComputeNode* node) {
    ImGui::InputFloat("result", &node->outputs_by_name.at("result").value, 0.0f, 0.0f, "%0.2f", ImGuiInputTextFlags_ReadOnly);
}

ComputeNode MakeMulOp(const string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Mul";

    node.AddInput(Attribute("a", AttributeType::UserDefined));
    node.AddInput(Attribute("b", AttributeType::UserDefined));

    node.AddOutput(Attribute("result", AttributeType::UserDefined));

    node.evaluate.connect<&MulOpEvaluateFn>();
    node.render_context_ui.connect<&MulOpRenderFn>();

    return node;
}

}