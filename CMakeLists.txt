cmake_minimum_required(VERSION 3.22)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(nncc
        VERSION 0.0.1
        LANGUAGES C CXX
        DESCRIPTION "To be made"
        )

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_PREFIX_PATH "external/torch")
find_package(Torch REQUIRED)

find_package(OpenCV REQUIRED)
find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)


set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS})
set(CMAKE_LINKER_FLAGS -fsanitize=address)

message("TORCH LIBS ${TORCH_LIBRARIES}")
message("TORCH INCLUDES ${TORCH_INCLUDE_DIRS}")
message("PYTHON_INCLUDE_DIRS ${Python_INCLUDE_DIRS}")

include(cmake/imgui.cmake)
add_subdirectory("external/bgfx.cmake")

add_subdirectory("external/glfw")
include_directories("external/glfw/include")

set(NNCC_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/lib")
add_subdirectory(${NNCC_COMMON_DIR})
include_directories(${NNCC_COMMON_DIR})

add_subdirectory(examples)

