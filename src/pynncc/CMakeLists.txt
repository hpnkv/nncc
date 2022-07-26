#pybind11_add_module(pynncc module.cpp)

add_library(
        pynncc
        module.cpp
        ${NNCC_PROJECT_ROOT}/src/pynncc/torch/tensor_registry.cpp
        ${NNCC_PROJECT_ROOT}/src/pynncc/torch/shm_communication.cpp
        ${TORCH_SRC_ROOT}/torch/lib/libshm/core.cpp
)

message(Python_INCLUDE_DIRS ${Python_INCLUDE_DIRS})


target_link_libraries(pynncc PRIVATE nncc "${TORCH_LIBRARIES}" ${Python_LIBRARIES} pybind11::embed ${CPP_REDIS_LIBS}
        "${TORCH_BUILD_ROOT}/lib/libonnx.a" "${TORCH_BUILD_ROOT}/lib/libonnx_proto.a"
        )
target_include_directories(
        pynncc PUBLIC
        ${Python_INCLUDE_DIRS}
        ${TORCH_ALL_INCLUDES}
        ${TORCH_SRC_ROOT}/torch/lib # provides "libshm/libshm.h"
)

if (APPLE)
    target_link_libraries(pynncc PUBLIC
            "-framework Accelerate"
            "-framework AppKit"
            "-framework Metal"
            "-framework MetalPerformanceShaders"
            "-framework MetalPerformanceShadersGraph"
    )
endif ()
