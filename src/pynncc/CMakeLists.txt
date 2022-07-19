pybind11_add_module(pynncc module.cpp)

add_library(
        pynnccv
        module.cpp
        ${NNCC_PROJECT_ROOT}/src/pynncc/torch/tensor_registry.cpp
)

target_link_libraries(pynncc PRIVATE nncc ${TORCH_LIBRARIES} ${Python_LIBRARIES} pybind11::embed shm)
target_include_directories(
        pynncc PRIVATE
        ${CMAKE_BINARY_DIR}/aten/src # provides "ATen/TypeExtendedInterface.h" to ATen.h
        ${TORCH_ROOT}/torch/lib # provides "libshm/libshm.h"
)