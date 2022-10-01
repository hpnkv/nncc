#include <pybind11/embed.h>

#include <pynncc/torch/tensor_registry.h>
#include <pynncc/torch/shm_communication.h>

#include <imgui_internal.h>

using namespace nncc;
namespace py = pybind11;

void SayHi() {
    auto& context = *context::Context::Get();
    py::print("Hello, world!");
}

PYBIND11_MODULE(pynnccp, m) {
    m.def("say_hi", &SayHi);
}