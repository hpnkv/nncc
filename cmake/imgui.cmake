set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
set(IMGUI_SOURCES "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"

        "${IMGUI_DIR}/backends/imgui_impl_metal.mm"
        "${IMGUI_DIR}/backends/imgui_impl_osx.mm")

add_library("imgui" ${IMGUI_SOURCES})
target_include_directories("imgui" PRIVATE "${IMGUI_DIR}")