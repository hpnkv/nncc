#include "graph.h"

namespace nncc::nodes {

int Attribute::id_counter = 0;
int ComputeNode::id_counter = 0;

ComputeNode MakeConstOp(const string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Const";

    node.AddSetting(Attribute("value", FloatValue{}));
    node.AddOutput(Attribute("value", FloatValue{}));

    auto evaluate_fn = [](ComputeNode* node, entt::registry* registry) {
        node->outputs_by_name.at("value").value = node->settings_by_name.at("value").value;
        return Result{0, ""};
    };
    node.evaluate.connect<evaluate_fn>();

    auto render_extended_view_fn = [](ComputeNode* node) {
        auto& value = node->settings_by_name.at("value").value;
        ImGui::InputFloat("Value", static_cast<float*>(value.Edit()), 0.1f, 1.0f, "%0.2f");
    };
    node.render_context_ui.connect<render_extended_view_fn>();

    return node;
}

ComputeNode MakeAddOp(const string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Add";

    node.AddInput(Attribute("a", FloatValue{}));
    node.AddInput(Attribute("b", FloatValue{}));

    node.AddOutput(Attribute("result", FloatValue{}));

    auto evaluate_fn = [](ComputeNode* node, entt::registry* registry) {
        auto a = node->inputs_by_name.at("a").value;
        auto b = node->inputs_by_name.at("b").value;

        node->outputs_by_name.at("result").value = a.Add(b);
        return Result{0, ""};
    };
    node.evaluate.connect<evaluate_fn>();

    auto render_extended_view_fn = [](ComputeNode* node) {
        auto& value = node->settings_by_name.at("result").value;
        ImGui::InputFloat("result", static_cast<float*>(value.Edit()), 0.0f, 0.0f, "%0.2f", ImGuiInputTextFlags_ReadOnly);
    };
    node.render_context_ui.connect<render_extended_view_fn>();

    return node;
}

ComputeNode MakeMulOp(const string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Mul";

    node.AddInput(Attribute("a", FloatValue{}));
    node.AddInput(Attribute("b", FloatValue{}));

    node.AddOutput(Attribute("result", FloatValue{}));

    auto evaluate_fn = [](ComputeNode* node, entt::registry* registry) {
        auto a = node->inputs_by_name.at("a").value;
        auto b = node->inputs_by_name.at("b").value;

        // TODO: it's a Mul, not Add
        node->outputs_by_name.at("result").value = a.Add(b);
        return Result{0, ""};
    };
    node.evaluate.connect<evaluate_fn>();

    auto render_extended_view_fn = [](ComputeNode* node) {
        auto& value = node->settings_by_name.at("result").value;
        ImGui::InputFloat("result", static_cast<float*>(value.Edit()), 0.0f, 0.0f, "%0.2f", ImGuiInputTextFlags_ReadOnly);
    };
    node.render_context_ui.connect<render_extended_view_fn>();

    return node;
}

}