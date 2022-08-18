#include "graph.h"

namespace nncc::nodes {

int Attribute::id_counter = 0;
int ComputeNode::id_counter = 0;

ComputeNode MakeConstOp(const nncc::string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Const";

    node.AddSetting(Attribute("value", AttributeType::UserDefined));
    node.AddOutput(Attribute("value", AttributeType::UserDefined));

    auto evaluate_fn = [](ComputeNode* node, entt::registry* registry) {
        node->outputs_by_name.at("value").value = node->settings_by_name.at("value").value;
        return Result{0, ""};
    };
    node.evaluate.connect<evaluate_fn>();

    auto render_extended_view_fn = [](ComputeNode* node) {
        ImGui::InputFloat("Value", &node->settings_by_name.at("value").value, 0.1f, 1.0f, "%0.2f");
    };
    node.render_context_ui.connect<render_extended_view_fn>();

    return node;
}

ComputeNode MakeAddOp(const nncc::string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Add";

    node.AddInput(Attribute("a", AttributeType::UserDefined));
    node.AddInput(Attribute("b", AttributeType::UserDefined));

    node.AddOutput(Attribute("result", AttributeType::UserDefined));

    auto evaluate_fn = [](ComputeNode* node, entt::registry* registry) {
        auto a = node->inputs_by_name.at("a").value;
        auto b = node->inputs_by_name.at("b").value;

        node->outputs_by_name.at("result").value = a + b;

        return Result{0, ""};
    };
    node.evaluate.connect<evaluate_fn>();

    auto render_extended_view_fn = [](ComputeNode* node) {
        ImGui::InputFloat("result", &node->outputs_by_name.at("result").value, 0.0f, 0.0f, "%0.2f", ImGuiInputTextFlags_ReadOnly);
    };
    node.render_context_ui.connect<render_extended_view_fn>();

    return node;
}

ComputeNode MakeMulOp(const nncc::string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Mul";

    node.AddInput(Attribute("a", AttributeType::UserDefined));
    node.AddInput(Attribute("b", AttributeType::UserDefined));

    node.AddOutput(Attribute("result", AttributeType::UserDefined));

    auto evaluate_fn = [](ComputeNode* node, entt::registry* registry) {
        auto a = node->inputs_by_name.at("a").value;
        auto b = node->inputs_by_name.at("b").value;

        node->outputs_by_name.at("result").value = a * b;

        return Result{0, ""};
    };
    node.evaluate.connect<evaluate_fn>();

    auto render_extended_view_fn = [](ComputeNode* node) {
        ImGui::InputFloat("result", &node->outputs_by_name.at("result").value, 0.0f, 0.0f, "%0.2f", ImGuiInputTextFlags_ReadOnly);
    };
    node.render_context_ui.connect<render_extended_view_fn>();

    return node;
}

}