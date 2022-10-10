find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

add_library(
        nncc_python SHARED
        ${NNCC_PROJECT_ROOT}/src/pynncc/compute/init.cpp
        ${NNCC_PROJECT_ROOT}/src/pynncc/compute/python_nodes.cpp
)

target_link_libraries(nncc_python PUBLIC nncc ${Python_LIBRARIES} pybind11::embed ${CPP_REDIS_LIBS})
target_include_directories(nncc_python PUBLIC ${Python_INCLUDE_DIRS})

add_subdirectory(torch)