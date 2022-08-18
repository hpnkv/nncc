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

list(APPEND CMAKE_PREFIX_PATH ${NNCC_PROJECT_ROOT}/external/dyno/dependencies/install)
set(DYNO_INCLUDE_PATH ${NNCC_PROJECT_ROOT}/external/dyno/include)

# PyTorch
set(TORCH_SRC_ROOT ${NNCC_PROJECT_ROOT}/external/pytorch)  # for libshm
set(TORCH_BUILD_ROOT ${NNCC_PROJECT_ROOT}/external/pytorch-build)
set(TORCH_INSTALL_ROOT ${NNCC_PROJECT_ROOT}/external/pytorch-install)

list(PREPEND CMAKE_PREFIX_PATH ${TORCH_INSTALL_ROOT})
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

if(MSVC)
    set(LIBSHM_SUBDIR libshm_windows)
else()
    set(LIBSHM_SUBDIR libshm)
endif()

set(LIBSHM_SRCDIR ${TORCH_SRC_ROOT}/torch/lib/${LIBSHM_SUBDIR})

find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

set(TORCH_ALL_INCLUDES ${TORCH_INSTALL_ROOT}/include ${TORCH_INSTALL_ROOT}/include/torch/csrc/api/include ${LIBSHM_SRCDIR})


add_subdirectory(src/pynncc)

message(TORCH_ALL_INCLUDES ${TORCH_ALL_INCLUDES})
#add_executable(test-poly ${NNCC_PROJECT_ROOT}/src/pynncc/torch/compute_nodes.cpp)
#target_link_libraries(test-poly PRIVATE nncc pynncc "${TORCH_LIBRARIES}" ${Python_LIBRARIES} pybind11::embed ${CPP_REDIS_LIBS}
#        "${TORCH_BUILD_ROOT}/lib/libonnx.a" "${TORCH_BUILD_ROOT}/lib/libonnx_proto.a")
#target_include_directories(test-poly PRIVATE ${TORCH_ALL_INCLUDES})
