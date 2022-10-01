pybind11_add_module(
        pynnccp
        module.cpp
)

add_library(
        pynncc SHARED
        module.cpp
        ${NNCC_PROJECT_ROOT}/src/pynncc/torch/tensor_registry.cpp
        ${NNCC_PROJECT_ROOT}/src/pynncc/torch/shm_communication.cpp
        ${TORCH_SRC_ROOT}/torch/lib/libshm/core.cpp
)
#set_target_properties(pynncc PROPERTIES CXX_VISIBILITY_PRESET hidden)

message(Python_INCLUDE_DIRS ${Python_INCLUDE_DIRS})


target_link_libraries(pynncc PRIVATE nncc ${Python_LIBRARIES} pybind11::embed ${CPP_REDIS_LIBS}
        "${TORCH_BUILD_ROOT}/lib/libonnx.a" "${TORCH_BUILD_ROOT}/lib/libonnx_proto.a"
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
target_link_libraries(pynnccp PRIVATE nncc-context pynncc nncc)
target_include_directories(
        pynncc PUBLIC
        ${Python_INCLUDE_DIRS}
        ${TORCH_ALL_INCLUDES}
        ${TORCH_SRC_ROOT}/torch/lib # provides "libshm/libshm.h"
)
target_include_directories(
        pynnccp PUBLIC
        ${BGFX_DIR}/include ${BX_DIR}/include
        ${CPP_REDIS_INCLUDES}
        ${folly_INCLUDE_DIRS}
        ${NNCC_PROJECT_ROOT}/src
        ${Python_INCLUDE_DIRS}
        ${TORCH_ALL_INCLUDES}
        ${TORCH_SRC_ROOT}/torch/lib # provides "libshm/libshm.h"
)
