#include <pybind11/pybind11.h>

#include <torch/torch.h>

#include <nncc/engine/loop.h>
#include <nncc/context/context.h>
#include <nncc/context/event.h>

namespace py = pybind11;

class PythonNNCCWrapper {
public:
    PythonNNCCWrapper() : context_(nncc::context::Context::Get()) {

    }

private:
    nncc::context::Context& context_;
};

PYBIND11_MODULE(nncc_python, m) {
    m.doc() = "nncc"; // optional module docstring

    py::class_<PythonNNCCWrapper>(m, "NNCCWrapper")
            .def(py::init<>());
}