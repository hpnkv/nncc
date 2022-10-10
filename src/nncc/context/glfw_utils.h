#pragma once

#include <memory>
#include <unordered_map>

#include <nncc/common/types.h>
#include <nncc/common/platform.h>
#include <nncc/input/hid.h>

#include <GLFW/glfw3.h>

#if NNCC_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#elif NNCC_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif NNCC_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include <GLFW/glfw3native.h>

#if NNCC_PLATFORM_LINUX

#include <nncc/common/X11_undefine_none.h>

#endif

#if NNCC_PLATFORM_OSX
void ToggleFullscreenCocoa(void* window);
#endif

namespace nncc::context {

struct GLFWWindowDeleter {
    void operator()(GLFWwindow* ptr) {
        glfwDestroyWindow(ptr);
    }
};


using GLFWWindowUniquePtr = std::unique_ptr<GLFWwindow, GLFWWindowDeleter>;


struct GLFWWindowWrapper {
    GLFWWindowWrapper(uint16_t _width, uint16_t _height, nncc::string _title, GLFWmonitor* monitor = nullptr,
                      GLFWwindow* share = nullptr) : title(std::move(_title)), width(_width), height(_height) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#if NNCC_PLATFORM_LINUX || NNCC_PLATFORM_BSD
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#endif
        auto window = glfwCreateWindow(
                width, height, title.c_str(), monitor, share
        );
        glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

        int temp_width, temp_height;
        glfwGetWindowSize(window, &temp_width, &temp_height);
        width = temp_width;
        height = temp_height;

#if NNCC_PLATFORM_LINUX || NNCC_PLATFORM_BSD
        auto actual_monitor = glfwGetWindowMonitor(window);
        float ignore;
        glfwGetMonitorContentScale(actual_monitor, &scale, &ignore);
#elif NNCC_PLATFORM_OSX
        scale = static_cast<float>(framebuffer_width) / static_cast<float>(width);
#endif

        ptr.reset(window);
    }

    void* GetNativeDisplayType() {
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
        return glfwGetX11Display();
#else
        return nullptr;
#endif
    }

    void* GetNativeHandle() {
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
        return (void*) (uintptr_t) glfwGetX11Window(ptr.get());
#elif BX_PLATFORM_OSX
        return glfwGetCocoaWindow(ptr.get());
#elif BX_PLATFORM_WINDOWS
        return glfwGetWin32Window(ptr.get());
#endif
    }

    GLFWWindowUniquePtr ptr;
    uint16_t width = 0, height = 0;
    int framebuffer_width = 0, framebuffer_height = 0;
    float scale;
    nncc::string title = "window";

    // TODO: do we need to save monitor and share?
};


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

    nncc::string title;
    int16_t idx;
};

}

namespace nncc::input {

uint8_t translateKeyModifiers(int glfw_modifiers);

std::unordered_map<int, input::Key> GlfwKeyTranslationTable();

input::MouseButton translateGlfwMouseButton(int button);

}