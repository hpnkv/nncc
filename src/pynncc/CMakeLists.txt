#pybind11_add_module(pynncc module.cpp)

add_library(
        pynncc
        module.cpp
        ${NNCC_PROJECT_ROOT}/src/pynncc/torch/tensor_registry.cpp
        ${NNCC_PROJECT_ROOT}/src/pynncc/torch/shm_communication.cpp
)

message(TORCH_LIBRARIES ${TORCH_LIBRARIES})

target_link_libraries(pynncc PUBLIC
        nncc
        ${TORCH_LIBRARIES} shm
        ${Python_LIBRARIES} pybind11::embed ${CPP_REDIS_LIBS}
)
target_include_directories(
        pynncc PUBLIC
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