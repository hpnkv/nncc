cmake_minimum_required(VERSION 3.22)

project(nncc LANGUAGES CXX)
set(NNCC_PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.3" CACHE STRING "Minimum OS X deployment version")
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)


# Non-Conan externals
set(BGFX_LIBRARY_TYPE SHARED)
add_subdirectory(external/bgfx.cmake)
add_subdirectory(external/cpp_redis)
set(CPP_REDIS_INCLUDES ${NNCC_PROJECT_ROOT}/external/cpp_redis/includes ${NNCC_PROJECT_ROOT}/external/cpp_redis/tacopie/includes)

add_subdirectory(src/nncc)
add_subdirectory(src/pynncc)
add_subdirectory(examples)