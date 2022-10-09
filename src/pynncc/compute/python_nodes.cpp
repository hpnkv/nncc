#include "python_nodes.h"

#include <nncc/gui/imgui_folly.h>

namespace nncc::compute {

auto PythonCodeOpEvaluateFn(ComputeNode* node, entt::registry* registry) {
    auto& state = *node->StateAs<PythonCodeOpState>();
    auto& context = *context::Context::Get();

    auto result = std::string(py::str(py::eval("generate(None)")));
    context.log_message = result;

    return Result{0, ""};
}

void PythonCodeOpUpdateInputsOutputs(ComputeNode* node) {
    auto& state = *node->StateAs<PythonCodeOpState>();

    nncc::list<std::tuple<nncc::string, nncc::string>> required_inputs;
    nncc::list<std::tuple<nncc::string, nncc::string>> required_outputs;

    py::exec(state.code.c_str());

    py::list inputs = py::eval("NNCC_INPUTS");
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
        auto name = it->attr("__getitem__")(0).cast<std::string>();
        auto type = it->attr("__getitem__")(1).cast<std::string>();
        required_inputs.emplace_back(name, type);
    }

    py::list outputs = py::eval("NNCC_OUTPUTS");
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
        auto name = it->attr("__getitem__")(0).cast<std::string>();
        auto type = it->attr("__getitem__")(1).cast<std::string>();
        required_inputs.emplace_back(name, type);
    }

    nncc::list<nncc::string> new_inputs, new_outputs;
    std::unordered_map<nncc::string, Attribute> new_inputs_by_name, new_outputs_by_name;
    for (const auto& [name, type] : required_inputs) {
        new_inputs.push_back(name);
        auto attr_type = AttributeTypeFromString(type);
        if (node->inputs_by_name.contains(name) && node->inputs_by_name.at(name).type == attr_type) {
            new_inputs_by_name.insert_or_assign(name, std::move(node->inputs_by_name.at(name)));
            continue;
        }
        new_inputs_by_name.insert_or_assign(name, Attribute(name, attr_type));
    }

    for (const auto& [name, type] : required_outputs) {
        new_outputs.push_back(name);
        auto attr_type = AttributeTypeFromString(type);
        if (node->outputs_by_name.contains(name) && node->outputs_by_name.at(name).type == attr_type) {
            new_outputs_by_name.insert_or_assign(name, std::move(node->inputs_by_name.at(name)));
            continue;
        }
        new_outputs_by_name.insert_or_assign(name, Attribute(name, attr_type));
    }

    node->inputs = std::move(new_inputs);
    node->inputs_by_name = std::move(new_inputs_by_name);

    node->outputs = std::move(new_outputs);
    node->outputs_by_name = std::move(new_outputs_by_name);
}

auto PythonCodeOpRenderFn(ComputeNode* node) {
    auto& state = *node->StateAs<PythonCodeOpState>();

    ImGui::InputTextMultiline("##pythoncode", &state.code);
    if (ImGui::Button("Update")) {
        PythonCodeOpUpdateInputsOutputs(node);
        return true;
    }

    return false;
}

ComputeNode MakePythonCodeOp(const void* _) {
    ComputeNode node;

    node.name = "Python code";
    node.type = "PythonCode";

    node.state = std::make_shared<PythonCodeOpState>();

    PythonCodeOpUpdateInputsOutputs(&node);

    node.evaluate.connect<&PythonCodeOpEvaluateFn>();
    node.render_context_ui.connect<&PythonCodeOpRenderFn>();

    return node;
}

}
