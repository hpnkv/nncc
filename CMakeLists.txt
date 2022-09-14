cmake_minimum_required(VERSION 3.22)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.3" CACHE STRING "Minimum OS X deployment version")
endif()

project(nncc)
set(NNCC_PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})


set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_MACOSX_RPATH 1)

# Non-Conan externals
add_subdirectory(external/bgfx.cmake)
add_subdirectory(external/cpp_redis)
set(CPP_REDIS_INCLUDES ${NNCC_PROJECT_ROOT}/external/cpp_redis/includes ${NNCC_PROJECT_ROOT}/external/cpp_redis/tacopie/includes)
set(CPP_REDIS_LIBS cpp_redis)

add_subdirectory(src/nncc)
add_subdirectory(examples)

# PyTorch
set(TORCH_SRC_ROOT ${NNCC_PROJECT_ROOT}/external/pytorch)  # for libshm
set(TORCH_BUILD_ROOT ${NNCC_PROJECT_ROOT}/external/pytorch-build)
set(TORCH_INSTALL_ROOT ${NNCC_PROJECT_ROOT}/external/pytorch-install)

list(PREPEND CMAKE_PREFIX_PATH ${TORCH_INSTALL_ROOT})

set(TORCH_CUDA_ARCH_LIST Common)
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS} -fopenmp")

if(MSVC)
    set(LIBSHM_SUBDIR libshm_windows)
else()
    set(LIBSHM_SUBDIR libshm)
endif()

list(APPEND CMAKE_BUILD_RPATH "${TORCH_INSTALL_ROOT}/lib")
list(APPEND CMAKE_INSTALL_RPATH "${TORCH_INSTALL_ROOT}/lib")

set(LIBSHM_SRCDIR ${TORCH_SRC_ROOT}/torch/lib/${LIBSHM_SUBDIR})

find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

set(TORCH_ALL_INCLUDES ${TORCH_INSTALL_ROOT}/include ${TORCH_INSTALL_ROOT}/include/torch/csrc/api/include ${LIBSHM_SRCDIR})

add_subdirectory(src/pynncc)
target_link_libraries(pynncc PRIVATE "${TORCH_LIBRARIES}")
