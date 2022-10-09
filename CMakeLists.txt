cmake_minimum_required(VERSION 3.22)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.3" CACHE STRING "Minimum OS X deployment version")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(nncc LANGUAGES CXX)
set(NNCC_PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})


set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# Non-Conan externals
set(BGFX_LIBRARY_TYPE SHARED)
add_subdirectory(external/bgfx.cmake)
add_subdirectory(external/cpp_redis)
set(CPP_REDIS_INCLUDES ${NNCC_PROJECT_ROOT}/external/cpp_redis/includes ${NNCC_PROJECT_ROOT}/external/cpp_redis/tacopie/includes)

add_subdirectory(src/nncc)

find_package(Python COMPONENTS Interpreter Development)
#set(PYBIND11_INSTALL_ROOT /home/alex/.virtualenvs/ml-experiments)
#list(PREPEND CMAKE_PREFIX_PATH ${PYBIND11_INSTALL_ROOT})
find_package(pybind11 REQUIRED)

add_subdirectory(src/pynncc)
add_subdirectory(examples)