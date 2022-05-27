#include "context.h"

#include <iostream>

namespace nncc::context {

bx::DefaultAllocator Context::allocator_;

bool Context::Init() {
    glfwSetErrorCallback(&Context::GLFWErrorCallback);

    if (!glfwInit()) {
        return false;
    }

    auto window_idx = CreateWindow(WindowParams());
    auto glfw_window = windows_[window_idx].get();

    bgfx::PlatformData pd;
    pd.ndt = GetNativeDisplayType();
    pd.nwh = GetNativeWindowHandle(glfw_window);
    pd.context = nullptr;
    pd.backBuffer = nullptr;
    pd.backBufferDS = nullptr;
    bgfx::setPlatformData(pd);

    return true;
}

int16_t Context::CreateWindow(const WindowParams& params) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(
            params.width, params.height, params.title.c_str(), params.monitor, params.share
    );

    glfwSetMouseButtonCallback(window, &Context::MouseButtonCallback);
    glfwSetCursorPosCallback(window, &Context::CursorPositionCallback);

    int16_t window_idx = static_cast<int16_t>(windows_.size());
    windows_.push_back(GLFWWindowUniquePtr(window));
    window_indices_[window] = window_idx;

    return window_idx;
}

void Context::DestroyWindow(int16_t idx) {
    window_indices_.erase(windows_[idx].get());
    windows_[idx].reset();
}

void Context::GLFWErrorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

void Context::MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t) {
    auto& context = Context::Get();

    double x_pos, y_pos;
    glfwGetCursorPos(window, &x_pos, &y_pos);

    std::unique_ptr<Event> event(new MouseEvent{
            .x = static_cast<int32_t>(x_pos),
            .y = static_cast<int32_t>(y_pos),
            .button = translateGlfwMouseButton(button),
            .down = action == GLFW_PRESS
    });
    event->type = EventType::MouseButton;

    auto window_idx = context.GetWindowIdx(window);
    context.GetEventQueue().Push(window_idx, std::move(event));
}

void Context::CursorPositionCallback(GLFWwindow* window, double x_pos, double y_pos) {
    auto& context = Context::Get();

    std::unique_ptr<Event> event(new MouseEvent{
            .x = static_cast<int32_t>(x_pos),
            .y = static_cast<int32_t>(y_pos),
    });
    event->type = EventType::MouseButton;

    auto window_idx = context.GetWindowIdx(window);
    context.GetEventQueue().Push(window_idx, std::move(event));
}

}