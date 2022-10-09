#pragma once

#include <nncc/compute/graph.h>

#include <pybind11/embed.h>

namespace py = pybind11;

namespace nncc::compute {

const char* kPythonGeneratorCode = R"(NNCC_INPUTS = [("a", "Float"), ("b", "Float")]
NNCC_OUTPUTS = [("result", "Float")]

def generate(node):
    return 0)";


struct PythonCodeOpState {
    nncc::string code{kPythonGeneratorCode};
    nncc::vector<std::tuple<nncc::string, nncc::string>> requested_inputs;
    nncc::vector<std::tuple<nncc::string, nncc::string>> requested_outputs;
};

struct GetSharedTensorOpState {
    nncc::string name;
    entt::entity entity = entt::null;
};

auto PythonCodeOpEvaluateFn(ComputeNode* node, entt::registry* registry);

auto PythonCodeOpRenderFn(ComputeNode* node);

ComputeNode MakePythonCodeOp(const void* _ = nullptr);

auto GetSharedTensorOpEvaluateFn(ComputeNode* node, entt::registry* registry);

auto GetSharedTensorOpRenderFn(ComputeNode* node);

ComputeNode MakeGetSharedTensorOp(const void* _ = nullptr);

}