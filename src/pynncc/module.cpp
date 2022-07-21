#include <stdexcept>
#include <pybind11/pybind11.h>
#include <libshm/libshm.h>
#include <torch/torch.h>

namespace py = pybind11;

void SayHi() {
    py::print("Hello, world!");
}

PYBIND11_MODULE(pynncc, m) {
    m.def("say_hi", &SayHi);
}