#pragma once

#include <cstdint>
#include <unordered_map>
#include <deque>
#include <vector>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/thread.h>

#define ENTT_USE_ATOMIC
#include <entt/entt.hpp>

#include <nncc/context/event.h>
#include <nncc/context/hid.h>
#include <nncc/context/glfw_utils.h>
#include <nncc/render/system.h>

namespace nncc::input {

struct InputSystem {
    EventQueue queue;

    std::deque<unsigned int> input_characters;
    KeyState key_state;
    MouseState mouse_state;

    bool ProcessEvents();
};

}

namespace nncc::context {

class Context {
public:
    Context() = default;

    static Context& Get() {
        static Context instance;
        return instance;
    }

    // We don't want occasional copies
    Context(const Context&) = delete;
    void operator=(const Context&) = delete;

    void Destroy() {
    }

    void Exit();

    bool Init();

    bool InitWindowing(GLFWerrorfun error_callback);

    int16_t CreateWindow(uint16_t width, uint16_t height, const nncc::string& title = "window", GLFWmonitor* monitor = nullptr,
                         GLFWwindow* share = nullptr);

    void DestroyWindow(int16_t idx);

    GLFWwindow* GetGlfwWindow(int16_t window_idx) {
        return windows_[window_idx].ptr.get();
    }

    GLFWWindowWrapper& GetWindow(int16_t window_idx) {
        return windows_[window_idx];
    }

    int16_t GetWindowIdx(GLFWwindow* window) {
        return window_indices_.at(window);
    }

    folly::ProducerConsumerQueue<GlfwMessage>& GetMessageQueue() {
        return glfw_message_queue_;
    }

    bx::Thread* GetDefaultThread() {
        return &default_thread_;
    }

    void SetWindowResolution(int16_t idx, int width, int height) {
        windows_[idx].width = width;
        windows_[idx].height = height;
    }

    entt::registry registry;
    entt::dispatcher dispatcher;
    input::InputSystem input;
    render::RenderingSystem rendering;

private:
    folly::ProducerConsumerQueue<GlfwMessage> glfw_message_queue_{64};

    nncc::vector<GLFWWindowWrapper> windows_;
    std::unordered_map<GLFWwindow*, int16_t> window_indices_;

    bx::Thread default_thread_;

    static void GLFWErrorCallback(int error, const char* description);

    static void MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t /* modifiers */);

    static void CharacterCallback(GLFWwindow* window, unsigned int codepoint);

    static void CursorPositionCallback(GLFWwindow* window, double x_pos, double y_pos);

    static void KeyCallback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t modifiers);
};

}