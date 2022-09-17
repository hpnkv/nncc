add_executable(003-imgui-demo main.cpp)
target_link_libraries(003-imgui-demo nncc)
target_include_directories(003-imgui-demo PRIVATE ${NNCC_INCLUDES})

if (IOS OR WIN32)
    add_custom_command(TARGET 003-imgui-demo COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/src/nncc/shaders $<TARGET_FILE_DIR:003-imgui-demo>/shaders)
else ()
    # For everything else symlink some folders into our output directory
    add_custom_command(TARGET 003-imgui-demo COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/src/nncc/shaders $<TARGET_FILE_DIR:003-imgui-demo>/shaders)
endif ()