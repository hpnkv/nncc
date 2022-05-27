#include "entry.h"

PYBIND11_MODULE(nncc, m) {
    m.doc() = "array visualiser"; // optional module docstring
    m.def("spawn_api_thread_and_run_render", &Run);
}
