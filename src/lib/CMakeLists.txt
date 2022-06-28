file(GLOB_RECURSE NNCC_COMMON_SOURCES ${NNCC_COMMON_DIR}/*.cpp)

set(
        NNCC_ENGINE_SOURCES
        nncc/engine/world_math.cpp
        nncc/engine/world_object.cpp
        nncc/engine/camera.cpp
        nncc/engine/loop.cpp
        nncc/common/image.cpp
        nncc/context/context.cpp
        nncc/context/glfw_utils.cpp
        nncc/python/tensor_registry.cpp
        nncc/python/shm_communication.cpp
        3rdparty/imgui_smart_strings.cpp
        3rdparty/bgfx_imgui/imgui/imgui.cpp
)

set(
        NNCC_INPUT_SOURCES
        nncc/input/hid.cpp
        nncc/input/event.cpp
#        nncc/input/input.cpp
)

set(
        NNCC_RENDER_SOURCES
        nncc/render/bgfx/memory.cpp
        nncc/render/renderer.cpp
        nncc/render/batch_renderer.cpp
        nncc/render/surface.cpp
        nncc/render/rendering.cpp
)

set(
        NNCC_SOURCES
        ${NNCC_INPUT_SOURCES}
        ${NNCC_ENGINE_SOURCES}
        ${NNCC_RENDER_SOURCES}
)

add_library(nncc-common ${NNCC_SOURCES})

target_link_libraries(nncc-common PUBLIC
        bgfx bx
        glfw
        imgui
        ${TORCH_LIBRARIES} ${Python_LIBRARIES} pybind11::embed shm
        ${FOLLY_LIBRARIES}
        ${CPP_REDIS_DIR}/lib/libcpp_redis.a ${CPP_REDIS_DIR}/lib/libtacopie.a
        )
target_include_directories(nncc-common PUBLIC
        ${BGFX_DIR}/include ${BX_DIR}/include ${BIMG_DIR}/include ${IMGUI_WRAPPER_DIR}
        ${NNCC_COMMON_DIR} ${Python_INCLUDE_DIRS}
        ${TORCH_INCLUDE_DIRS} ${pybind11_INCLUDE_DIR}
        ${FOLLY_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/aten/src # provides "ATen/TypeExtendedInterface.h" to ATen.h
        ${TORCH_ROOT}/torch/lib # provides "libshm/libshm.h"
        ${CPP_REDIS_DIR}/include
        )

if (APPLE)
    target_include_directories(nncc-common PUBLIC "-framework Metal" "-framework AppKit" "-framework Foundation" "-framework Quartz")
endif ()

