add_executable(099-tensor-exchange main.cpp)

target_link_libraries(099-tensor-exchange PRIVATE nncc nncc_python)
target_include_directories(099-tensor-exchange PRIVATE ${NNCC_INCLUDES} ${TORCH_ALL_INCLUDES})

if (IOS OR WIN32)
    add_custom_command(TARGET 099-tensor-exchange COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/src/nncc/nncc_shaders $<TARGET_FILE_DIR:099-tensor-exchange>/nncc_shaders)
else ()
    # For everything else symlink some folders into our output directory
    add_custom_command(TARGET 099-tensor-exchange COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/src/nncc/nncc_shaders $<TARGET_FILE_DIR:099-tensor-exchange>/nncc_shaders)
endif ()