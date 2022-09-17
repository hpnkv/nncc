add_executable(002-base-context main.cpp)
target_link_libraries(002-base-context nncc)
target_include_directories(002-base-context PRIVATE ${NNCC_INCLUDES})

if (IOS OR WIN32)
    add_custom_command(TARGET 002-base-context COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/src/nncc/nncc_shaders $<TARGET_FILE_DIR:002-base-context>/nncc_shaders)
else ()
    # For everything else symlink some folders into our output directory
    add_custom_command(TARGET 002-base-context COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/src/nncc/nncc_shaders $<TARGET_FILE_DIR:002-base-context>/nncc_shaders)
endif ()