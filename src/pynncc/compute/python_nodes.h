#pragma once

#include <nncc/compute/graph.h>

#include <pybind11/embed.h>

namespace py = pybind11;

namespace nncc::compute {

const char* kPythonGeneratorCode = R"(def generate(node):
    return 0)";


struct PythonCodeOpState {
    nncc::string code{kPythonGeneratorCode};
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