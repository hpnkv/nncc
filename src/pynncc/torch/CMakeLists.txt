set(CMAKE_CUDA_ARCHITECTURES OFF)
set(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
set(CMAKE_CUDA_STANDARD 14)

# PyTorch
set(TORCH_SRC_ROOT ${NNCC_PROJECT_ROOT}/external/pytorch)  # for libshm
set(TORCH_BUILD_ROOT ${NNCC_PROJECT_ROOT}/external/pytorch-build)
set(TORCH_INSTALL_ROOT ${NNCC_PROJECT_ROOT}/external/pytorch-install)

list(PREPEND CMAKE_PREFIX_PATH ${TORCH_INSTALL_ROOT})

set(TORCH_CUDA_ARCH_LIST Common)
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

if(MSVC)
    set(LIBSHM_SUBDIR libshm_windows)
else()
    set(LIBSHM_SUBDIR libshm)
endif()

list(APPEND CMAKE_BUILD_RPATH "${TORCH_INSTALL_ROOT}/lib")
list(APPEND CMAKE_INSTALL_RPATH "${TORCH_INSTALL_ROOT}/lib")

set(LIBSHM_SRCDIR ${TORCH_SRC_ROOT}/torch/lib/${LIBSHM_SUBDIR})
set(TORCH_ALL_INCLUDES ${TORCH_INSTALL_ROOT}/include ${TORCH_INSTALL_ROOT}/include/torch/csrc/api/include ${LIBSHM_SRCDIR})

add_library(
        nncc_pytorch SHARED
        ${NNCC_PROJECT_ROOT}/src/pynncc/torch/tensor_registry.cpp
        ${NNCC_PROJECT_ROOT}/src/pynncc/compute/pytorch_nodes.cpp
        ${TORCH_SRC_ROOT}/torch/lib/libshm/core.cpp
)
set_property(TARGET nncc_pytorch PROPERTY LINK_DEPENDS_NO_SHARED ON)
#set_property(TARGET nncc_pytorch PROPERTY CXX_VISIBILITY_PRESET hidden)
#set_property(TARGET nncc_pytorch PROPERTY VISIBILITY_INLINES_HIDDEN ON)

target_link_libraries(nncc_pytorch PRIVATE nncc_python ${TORCH_LIBRARIES} cpp_redis)
target_link_libraries(nncc_pytorch INTERFACE ${TORCH_LIBRARIES})
target_link_libraries(nncc_pytorch PRIVATE "${TORCH_BUILD_ROOT}/lib/libonnx.a" "${TORCH_BUILD_ROOT}/lib/libonnx_proto.a")
target_include_directories(
        nncc_pytorch PUBLIC
        ${Python_INCLUDE_DIRS}
        ${TORCH_ALL_INCLUDES}
        ${TORCH_SRC_ROOT}/torch/lib # provides "libshm/libshm.h"
)

if (APPLE)
    target_link_libraries(nncc_pytorch PUBLIC
            "-framework Accelerate"
            "-framework AppKit"
            "-framework Metal"
            "-framework MetalPerformanceShaders"
            "-framework MetalPerformanceShadersGraph"
            )
endif ()