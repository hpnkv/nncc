#pragma once

#include <nncc/compute/graph.h>

#include <pybind11/embed.h>

namespace py = pybind11;

namespace nncc::compute {

const char* kPythonBootstrapCode = R"(from dataclasses import dataclass

@dataclass
class NNCCNode:
    inputs: dict
    outputs: dict

    def __init__(self, inputs: dict = None, outputs: dict = None):
        self.inputs = inputs if inputs is not None else dict()
        self.outputs = outputs if outputs is not None else dict()
)";

const char* kPythonGeneratorCode = R"(NNCC_INPUTS = [("a", "Float"), ("b", "Float")]
NNCC_OUTPUTS = [("result", "Float")]

def process(node: NNCCNode):
    return 0)";


struct PythonCodeOpState {
    nncc::string code{kPythonGeneratorCode};
    py::dict globals;
};

py::handle MakeNodeInterface(ComputeNode* node);

auto PythonCodeOpEvaluateFn(ComputeNode* node, entt::registry* registry);

auto PythonCodeOpRenderFn(ComputeNode* node);

ComputeNode MakePythonCodeOp(const void* _ = nullptr);

}

namespace nncc::python {

void init();

}