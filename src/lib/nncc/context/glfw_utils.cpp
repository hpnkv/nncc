#include "glfw_utils.h"
#include "hid.h"

namespace nncc::context {

MouseButton translateGlfwMouseButton(int button) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        return MouseButton::Left;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        return MouseButton::Right;
    }

    return MouseButton::None;
}

void* GetNativeDisplayType() {
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    return glfwGetX11Display();
#else
    return nullptr;
#endif
}

void* GetNativeWindowHandle(GLFWwindow* window) {
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    init.platformData.ndt = glfwGetX11Display();
        return (void*)(uintptr_t)glfwGetX11Window(window);
#elif BX_PLATFORM_OSX
    return glfwGetCocoaWindow(window);
#elif BX_PLATFORM_WINDOWS
    return glfwGetWin32Window(window);
#endif
}

}