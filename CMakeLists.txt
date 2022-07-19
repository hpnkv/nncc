cmake_minimum_required(VERSION 3.22)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version")
endif()

project(nncc)
set(NNCC_PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
conan_basic_setup()

find_package(glfw3)

# Non-Conan externals
add_subdirectory(external/bgfx.cmake)
add_subdirectory(external/cpp_redis)
set(CPP_REDIS_INCLUDES ${NNCC_PROJECT_ROOT}/external/cpp_redis/includes ${NNCC_PROJECT_ROOT}/external/cpp_redis/tacopie/includes)
set(CPP_REDIS_LIBS cpp_redis tacopie)

add_subdirectory(src/nncc)
add_subdirectory(examples)

# PyTorch
list(APPEND CMAKE_PREFIX_PATH "${NNCC_PROJECT_ROOT}/external/pytorch-install")
find_package(Torch REQUIRED)
if(MSVC)
    set(LIBSHM_SUBDIR libshm_windows)
else()
    set(LIBSHM_SUBDIR libshm)
endif()
set(TORCH_ROOT ${NNCC_PROJECT_ROOT}/external/pytorch)
set(LIBSHM_SRCDIR ${TORCH_ROOT}/torch/lib/${LIBSHM_SUBDIR})
add_subdirectory(${LIBSHM_SRCDIR})
find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

add_subdirectory(src/pynncc)
