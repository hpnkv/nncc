#pragma once

#include <cstdint>
#include <unordered_map>
#include <deque>
#include <vector>

#include <bgfx/bgfx.h>
#include <bx/thread.h>
#include <fmt/format.h>
#include <imgui.h>

#define ENTT_USE_ATOMIC
#include <entt/entt.hpp>

#include <nncc/input/input.h>
#include <nncc/context/glfw_utils.h>
#include <nncc/engine/timer.h>
#include <nncc/rendering/rendering.h>

namespace nncc::context {

class GlfwWindowingImpl {
public:
    GlfwWindowingImpl() = default;

    static GlfwWindowingImpl& Get();

    // We don't want occasional copies
    GlfwWindowingImpl(const GlfwWindowingImpl&) = delete;

    void operator=(const GlfwWindowingImpl&) = delete;

    static void GLFWErrorCallback(int error, const char* description);

    static void MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t /* modifiers */);

    static void CharacterCallback(GLFWwindow* window, unsigned int codepoint);

    static void CursorPositionCallback(GLFWwindow* window, double x_pos, double y_pos);

    static void KeyCallback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t modifiers);

    static void ScrollCallback(GLFWwindow* window, double x_offset, double y_offset);

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
};

class SubsystemManager {
public:
    template<class T>
    void Register(T* instance);

    template<class T>
    void Register(T* instance, const nncc::string& name) {
        subsystems_[name].push_back(static_cast<void*>(instance));
    }

    template<class T>
    T* Get() {
        nncc::string type_name(fmt::format("__{}", typeid(T).name()));
        if (subsystems_[type_name].empty()) {
            return nullptr;
        }
        return static_cast<T*>(subsystems_[type_name][0]);
    }

    template<class T>
    T* Get(const nncc::string& name) {
        if (!subsystems_.contains(name) || subsystems_[name].empty()) {
            return nullptr;
        }
        return static_cast<T*>(subsystems_[name][0]);
    }

    template<class T>
    nncc::vector<T*> GetAll() {
        nncc::string type_name(typeid(T).name());
        nncc::vector<T*> result;
        result.reserve(subsystems_[type_name].size());
        for (auto* instance: subsystems_[type_name]) {
            result.push_back(static_cast<T*>(instance));
        }
        return result;
    }

private:
    std::unordered_map<nncc::string, nncc::vector<void*>> subsystems_;
};


template<class T>
void SubsystemManager::Register(T* instance) {
    nncc::string type_name(fmt::format("__{}", typeid(T).name()));
    subsystems_[type_name].push_back(static_cast<void*>(instance));
}

struct ImGuiAllocators {
    ImGuiMemAllocFunc p_alloc_func;
    ImGuiMemFreeFunc p_free_func;
    void* user_data;
};

class __attribute__((visibility ("default"))) Context {
public:
    Context() = default;

    static __attribute__((visibility ("default"))) Context* Get();

    // We don't want occasional copies
    Context(const Context&) = delete;

    void operator=(const Context&) = delete;

    void Exit();

    bool InitInMainThread();

    bool InitWindowing(GLFWerrorfun error_callback);

    int16_t CreateWindow(uint16_t width,
                         uint16_t height,
                         const nncc::string& title = "window",
                         GLFWmonitor* monitor = nullptr,
                         GLFWwindow* share = nullptr);

    void DestroyWindow(int16_t idx);

    GLFWwindow* GetGlfwWindow(int16_t window_idx);

    GLFWWindowWrapper& GetWindow(int16_t window_idx);

    int16_t GetWindowIdx(GLFWwindow* window);

    bx::Thread* GetDefaultThread();

    void SetWindowSize(int16_t idx, int width, int height);

    entt::registry registry;
    entt::dispatcher dispatcher;

    input::InputSystem input;
    engine::Timer timer;
    rendering::RenderingSystem rendering;
    SubsystemManager subsystems;

    nncc::string log_message;
    uint32_t frame_number = 0;
    ImGuiContext* imgui_context = nullptr;
    ImGuiAllocators imgui_allocators;

private:
    nncc::vector<GLFWWindowWrapper> windows_;
    std::unordered_map<GLFWwindow*, int16_t> window_indices_;

    bx::Thread default_thread_;
};

}