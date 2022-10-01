#include "graph.h"

#include <nncc/gui/imgui_folly.h>
#include <nncc/gui/gui.h>

#include <pybind11/embed.h>

namespace py = pybind11;

namespace nncc::compute {

int Attribute::id_counter = 0;
int ComputeNode::id_counter = 0;

struct ConstOpState {
    enum class Type {
        Float = 0,
        String = 1,
        Generated = 2,
        Count
    };
    static const char* options[];
    std::variant<float, nncc::string> value;
    Type type {Type::Float};
    bool edited = false;
};

const char* ConstOpState::options[] = {"Float", "String", "Generated"};

auto ConstOpEvaluateFn(ComputeNode* node, entt::registry* registry) {
    auto& state = *node->StateAs<ConstOpState>();
    node->outputs_by_name.at("value").value = state.value;

    auto& context = *context::Context::Get();

    if (state.type == ConstOpState::Type::Generated) {
         py::exec(std::get<nncc::string>(state.value).c_str());
         auto result = std::string(py::str(py::eval("generate(None)")));
         context.log_message = result;
    }

    return Result{0, ""};
}

const char* kPythonGeneratorCode = R"(def generate(node):
    return 0)";

auto ConstOpRenderFn(ComputeNode* node) {
    auto& state = *node->StateAs<ConstOpState>();
    const char* combo_preview_value = ConstOpState::options[static_cast<int>(state.type)];

    if (ImGui::BeginCombo("Type", combo_preview_value, 0)) {
        for (int n = 0; n < IM_ARRAYSIZE(ConstOpState::options); n++) {
            const bool is_selected = (static_cast<int>(state.type) == n);
            if (ImGui::Selectable(ConstOpState::options[n], is_selected))
                state.type = static_cast<ConstOpState::Type>(n);
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (state.type == ConstOpState::Type::Float) {
        node->outputs_by_name.at("value").type = AttributeType::Float;
    } else if (state.type == ConstOpState::Type::String) {
        node->outputs_by_name.at("value").type = AttributeType::String;
    } else {
        node->outputs_by_name.at("value").type = AttributeType::UserDefined;
    }

    if (state.type == ConstOpState::Type::Float) {
        if (!std::holds_alternative<float>(state.value)) {
            state.value = 0.0f;
        }
        if (ImGui::InputFloat("##constdouble", &std::get<float>(state.value), 0.01f, 0.1f, "%0.3f", 0)) {
            state.edited = true;
        }
    } else if (state.type == ConstOpState::Type::String) {
        if (!state.edited || !std::holds_alternative<nncc::string>(state.value)) {
            state.value = "value";
        }
        if (ImGui::InputText("##conststring", &std::get<nncc::string>(state.value))) {
            state.edited = true;
        }
    } else {
        if (!state.edited || !std::holds_alternative<nncc::string>(state.value)) {
            state.value = kPythonGeneratorCode;
        }
        if (ImGui::InputTextMultiline("##constgenerator", &std::get<nncc::string>(state.value))) {
            state.edited = true;
        }
    }
}

ComputeNode MakeConstOp(const string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Const";

    node.AddOutput(Attribute("value", AttributeType::Float));

    node.evaluate.connect<&ConstOpEvaluateFn>();
    node.render_context_ui.connect<&ConstOpRenderFn>();

    node.state = std::make_shared<ConstOpState>();

    return node;
}

auto AddOpEvaluateFn(ComputeNode* node, entt::registry* registry) {
    auto a = node->inputs_by_name.at("a").value;
    auto b = node->inputs_by_name.at("b").value;

    node->outputs_by_name.at("result").value = std::get<float>(a) + std::get<float>(b);

    return Result{0, ""};
}

auto AddOpRenderFn(ComputeNode* node) {
    ImGui::InputFloat("result", &std::get<float>(node->outputs_by_name.at("result").value), 0.0f, 0.0f, "%0.2f",
                      ImGuiInputTextFlags_ReadOnly);
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

    node->outputs_by_name.at("result").value = std::get<float>(a) * std::get<float>(b);

    return Result{0, ""};
}

auto MulOpRenderFn(ComputeNode* node) {
    ImGui::InputFloat("result", &std::get<float>(node->outputs_by_name.at("result").value), 0.0f, 0.0f, "%0.2f",
                      ImGuiInputTextFlags_ReadOnly);
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