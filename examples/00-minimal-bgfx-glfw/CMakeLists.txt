include(${PROJECT_SOURCE_DIR}/src/lib/nncc/render/shaders/shaders.cmake)

add_executable(00-minimal-bgfx-glfw main.cpp)
target_link_libraries(00-minimal-bgfx-glfw PRIVATE nncc-common)
target_include_directories(00-minimal-bgfx-glfw PRIVATE
        ${BGFX_DIR}/include ${BX_DIR}/include ${BIMG_DIR}/include
        ${IMGUI_WRAPPER_DIR}
        ${NNCC_COMMON_DIR}
        ${Python_INCLUDE_DIRS} ${TORCH_INCLUDE_DIRS} ${pybind11_INCLUDE_DIR}
        )

use_bgfx_shader(${PROJECT_SOURCE_DIR}/src/lib/nncc/render/shaders/default_diffuse/fs_default_diffuse.sc 00-minimal-bgfx-glfw)
use_bgfx_shader(${PROJECT_SOURCE_DIR}/src/lib/nncc/render/shaders/default_diffuse/vs_default_diffuse.sc 00-minimal-bgfx-glfw)

use_bgfx_shader(${PROJECT_SOURCE_DIR}/src/lib/3rdparty/bgfx_imgui/imgui/shaders/vs_imgui_image.sc 00-minimal-bgfx-glfw)
use_bgfx_shader(${PROJECT_SOURCE_DIR}/src/lib/3rdparty/bgfx_imgui/imgui/shaders/fs_imgui_image.sc 00-minimal-bgfx-glfw)
use_bgfx_shader(${PROJECT_SOURCE_DIR}/src/lib/3rdparty/bgfx_imgui/imgui/shaders/vs_ocornut_imgui.sc 00-minimal-bgfx-glfw)
use_bgfx_shader(${PROJECT_SOURCE_DIR}/src/lib/3rdparty/bgfx_imgui/imgui/shaders/fs_ocornut_imgui.sc 00-minimal-bgfx-glfw)

pybind11_add_module(nncc_python python.cpp)
target_link_libraries(nncc_python PRIVATE nncc-common ${TORCH_LIBRARIES})
target_include_directories(nncc_python PUBLIC ${NNCC_COMMON_DIR} ${Python_INCLUDE_DIRS} ${TORCH_INCLUDE_DIRS} ${pybind11_INCLUDE_DIR})

#use_bgfx_shader(${PROJECT_SOURCE_DIR}/src/lib/nncc/render/shaders/default_diffuse/fs_default_diffuse.sc 00-minimal-bgfx-glfw)
#use_bgfx_shader(${PROJECT_SOURCE_DIR}/src/lib/nncc/render/shaders/default_diffuse/vs_default_diffuse.sc 00-minimal-bgfx-glfw)