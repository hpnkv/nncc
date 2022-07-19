add_executable(001-helloworld-mt main.cpp)
target_link_libraries(001-helloworld-mt nncc)
target_include_directories(001-helloworld-mt PRIVATE ${NNCC_INCLUDES})

if (IOS OR WIN32)
    add_custom_command( TARGET 001-helloworld-mt COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/src/nncc/shaders $<TARGET_FILE_DIR:001-helloworld-mt>/shaders)
else()
    # For everything else symlink some folders into our output directory
    add_custom_command( TARGET 001-helloworld-mt COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/src/nncc/shaders $<TARGET_FILE_DIR:001-helloworld-mt>/shaders)
endif()