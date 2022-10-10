find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

add_library(
        nncc_python SHARED
        ${NNCC_PROJECT_ROOT}/src/pynncc/compute/init.cpp
        ${NNCC_PROJECT_ROOT}/src/pynncc/compute/python_nodes.cpp
)

set_property(TARGET nncc_python PROPERTY LINK_DEPENDS_NO_SHARED ON)
#set_property(TARGET nncc_python PROPERTY CXX_VISIBILITY_PRESET hidden)
#set_property(TARGET nncc_python PROPERTY VISIBILITY_INLINES_HIDDEN ON)

target_link_libraries(nncc_python PUBLIC nncc ${Python_LIBRARIES} pybind11::embed ${CPP_REDIS_LIBS})
target_include_directories(nncc_python PUBLIC ${Python_INCLUDE_DIRS})

add_subdirectory(torch)