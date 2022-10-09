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
};

auto PythonCodeOpEvaluateFn(ComputeNode* node, entt::registry* registry);

auto PythonCodeOpRenderFn(ComputeNode* node);

ComputeNode MakePythonCodeOp(const void* _ = nullptr);

}

namespace nncc::python {

void init();

}