#pragma once

#include <string>

#include <bx/platform.h>

#include <GLFW/glfw3.h>

#if BX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BX_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include <GLFW/glfw3native.h>

namespace nncc::context {

struct WindowParams {
    int width = 1024;
    int height = 768;
    std::string title = "window";
    GLFWmonitor* monitor = nullptr;
    GLFWwindow* share = nullptr;
};

void* GetNativeDisplayType();

void* GetNativeWindowHandle(GLFWwindow* window);

struct GLFWWindowDeleter{
    void operator()(GLFWwindow* ptr){
        glfwDestroyWindow(ptr);
    }
};

using GLFWWindowUniquePtr = std::unique_ptr<GLFWwindow, GLFWWindowDeleter>;

enum class GlfwMessageType {
    Create,
    Destroy,
    SetTitle,
    SetPosition,
    SetSize,
    ToggleFrame,
    ToggleFullscreen,
    MouseLock
};

struct GlfwMessage {
    GlfwMessageType type;

    int32_t x;
    int32_t y;

    uint32_t width;
    uint32_t height;
    uint32_t flags;
    bool value;

    std::string title;
    int16_t idx;
};

}