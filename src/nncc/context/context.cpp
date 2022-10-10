#include "context.h"
#include <bgfx/platform.h>

#include <iostream>

namespace nncc::context {

bool Context::InitInMainThread() {
    InitWindowing(&GlfwWindowingImpl::GLFWErrorCallback);
    input.Init();

    // this init happens prior to RenderingSystem.InitInMainThread() to indicate BGFX that
    // we will use a separate rendering thread.
    bgfx::renderFrame();
    return true;
}

bool Context::InitWindowing(GLFWerrorfun error_callback) {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        return false;
    }

    CreateWindow(1600, 1000, "window");
    return true;
}

int16_t Context::CreateWindow(uint16_t width, uint16_t height, const nncc::string& title, GLFWmonitor* monitor,
                              GLFWwindow* share) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = GLFWWindowWrapper(width, height, title, monitor, share);

    glfwSetMouseButtonCallback(window.ptr.get(), &GlfwWindowingImpl::MouseButtonCallback);
    glfwSetCursorPosCallback(window.ptr.get(), &GlfwWindowingImpl::CursorPositionCallback);
    glfwSetKeyCallback(window.ptr.get(), &GlfwWindowingImpl::KeyCallback);
    glfwSetCharCallback(window.ptr.get(), &GlfwWindowingImpl::CharacterCallback);
    glfwSetScrollCallback(window.ptr.get(), &GlfwWindowingImpl::ScrollCallback);
    glfwSetFramebufferSizeCallback(window.ptr.get(), &GlfwWindowingImpl::FramebufferSizeCallback);

    int16_t window_idx = static_cast<int16_t>(windows_.size());
    window_indices_[window.ptr.get()] = window_idx;
    windows_.push_back(std::move(window));

    return window_idx;
}

void Context::DestroyWindow(int16_t idx) {
    window_indices_.erase(windows_[idx].ptr.get());
    windows_[idx].ptr.reset();
}

void GlfwWindowingImpl::GLFWErrorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

void GlfwWindowingImpl::MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t mods) {
    double x_pos, y_pos;
    glfwGetCursorPos(window, &x_pos, &y_pos);

    auto& context = *Context::Get();

    input::MouseEvent event;
    event.x = static_cast<int32_t>(x_pos);
    event.y = static_cast<int32_t>(y_pos);
    event.button = input::translateGlfwMouseButton(button);
    event.down = action == GLFW_PRESS;
    event.modifiers = mods;
    event.window_idx = context.GetWindowIdx(window);
    event.type = input::MouseEventType::Button;

    context.dispatcher.enqueue(event);
}

void GlfwWindowingImpl::CursorPositionCallback(GLFWwindow* window, double x_pos, double y_pos) {
    auto& context = *Context::Get();

    input::MouseEvent event;
    event.x = static_cast<int32_t>(x_pos);
    event.y = static_cast<int32_t>(y_pos);
    event.window_idx = context.GetWindowIdx(window);
    event.type = input::MouseEventType::Move;

    context.dispatcher.enqueue(event);
}

const auto glfw_key_translation_table = input::GlfwKeyTranslationTable();

void GlfwWindowingImpl::KeyCallback(GLFWwindow* window, int32_t glfw_key, int32_t scancode, int32_t action,
                                    int32_t modifiers) {
    if (glfw_key == GLFW_KEY_UNKNOWN) {
        return;
    }

    int mods = input::translateKeyModifiers(modifiers);
    if (!glfw_key_translation_table.contains(glfw_key)) {
        return;
    }
    auto key = glfw_key_translation_table.at(glfw_key);

    auto& context = *Context::Get();

    input::KeyEvent event;
    event.key = key;
    event.modifiers = mods;
    event.down = action == GLFW_PRESS || action == GLFW_REPEAT;
    event.window_idx = context.GetWindowIdx(window);

    context.dispatcher.enqueue(event);
}

void GlfwWindowingImpl::CharacterCallback(GLFWwindow* window, unsigned int codepoint) {
    auto& context = *Context::Get();

    input::CharEvent event;
    event.codepoint = codepoint;
    event.window_idx = context.GetWindowIdx(window);

    context.dispatcher.enqueue(event);
}

void GlfwWindowingImpl::ScrollCallback(GLFWwindow* window, double x_offset, double y_offset) {
    auto& context = *Context::Get();

    input::MouseEvent event;
    event.type = input::MouseEventType::Scroll;
    event.window_idx = context.GetWindowIdx(window);

    event.scroll_x = x_offset;
    event.scroll_y = y_offset;

    context.dispatcher.enqueue(event);
}

void GlfwWindowingImpl::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {

}

GlfwWindowingImpl& GlfwWindowingImpl::Get() {
    static GlfwWindowingImpl instance;
    return instance;
}

void Context::Exit() {
    input::ExitEvent event;
    event.window_idx = 0;
    dispatcher.enqueue(event);
}

GLFWwindow* Context::GetGlfwWindow(int16_t window_idx) {
    return windows_[window_idx].ptr.get();
}

GLFWWindowWrapper& Context::GetWindow(int16_t window_idx) {
    return windows_[window_idx];
}

int16_t Context::GetWindowIdx(GLFWwindow* window) {
    return window_indices_.at(window);
}

bx::Thread* Context::GetDefaultThread() {
    return &default_thread_;
}

void Context::SetWindowSize(int16_t idx, int width, int height) {
    windows_[idx].width = width;
    windows_[idx].height = height;

    glfwGetFramebufferSize(windows_[idx].ptr.get(), &windows_[idx].framebuffer_width,
                           &windows_[idx].framebuffer_height);
}

Context* Context::Get() {
    static Context instance;
    return &instance;
}

bool Context::ShouldExit() const {
    return should_exit;
}

}
