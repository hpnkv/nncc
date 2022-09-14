add_executable(099-tensor-exchange main.cpp)

set(TORCH_CUDA_ARCH_LIST Common)
find_package(Torch REQUIRED)

target_link_libraries(099-tensor-exchange PRIVATE nncc pynncc ${Python_LIBRARIES})
target_include_directories(099-tensor-exchange PRIVATE ${NNCC_INCLUDES} ${TORCH_ALL_INCLUDES})

if (IOS OR WIN32)
    add_custom_command( TARGET 099-tensor-exchange COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/src/nncc/shaders $<TARGET_FILE_DIR:099-tensor-exchange>/shaders)
else()
    # For everything else symlink some folders into our output directory
    add_custom_command( TARGET 099-tensor-exchange COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/src/nncc/shaders $<TARGET_FILE_DIR:099-tensor-exchange>/shaders)

endif()