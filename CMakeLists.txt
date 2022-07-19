cmake_minimum_required(VERSION 3.22)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version")
endif()

project(nncc)
set(NNCC_PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
conan_basic_setup()

find_package(glfw3)

# Non-Conan externals
add_subdirectory(external/bgfx.cmake)
add_subdirectory(external/cpp_redis)
set(CPP_REDIS_INCLUDES ${NNCC_PROJECT_ROOT}/external/cpp_redis/includes ${NNCC_PROJECT_ROOT}/external/cpp_redis/tacopie/includes)
set(CPP_REDIS_LIBS cpp_redis tacopie)

#list(APPEND CMAKE_PREFIX_PATH "external/torch")
#find_package(Torch REQUIRED)
#if(MSVC)
#    set(LIBSHM_SUBDIR libshm_windows)
#else()
#    set(LIBSHM_SUBDIR libshm)
#endif()
#set(TORCH_ROOT /Users/apankov/Snapchat/dev/pytorch)
#set(LIBSHM_SRCDIR external/torch/lib/${LIBSHM_SUBDIR})
#add_subdirectory(${LIBSHM_SRCDIR})

add_subdirectory(src/nncc)
target_include_directories(nncc PRIVATE ${CPP_REDIS_INCLUDES})

add_subdirectory(examples)
