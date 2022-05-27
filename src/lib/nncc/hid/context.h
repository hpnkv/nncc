#pragma once

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/thread.h>

#include <GLFW/glfw3.h>

#if BX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BX_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include <GLFW/glfw3native.h>

namespace nncc::engine {

enum class MouseButton {
    None,
    Left,
    Middle,
    Right
};

MouseButton translateGlfwMouseButton(int button);

enum class EventType {
    Exit,
    Mouse,
    None
};

struct Event {
    int16_t window_idx = 0;
    EventType type = EventType::None;
};

struct MouseEvent : public Event {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;

    MouseButton button = MouseButton::None;

    bool down = false;
    bool move = false;
};

struct KeyEvent : public Event {
};

struct ExitEvent : public Event {
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

    std::string title;
    int16_t idx;
};

class EventQueue {
public:
    explicit EventQueue(bx::AllocatorI* allocator) : queue_(allocator) {}

    std::unique_ptr<Event> Poll() {
        void* raw_pointer = queue_.pop();
        if (raw_pointer == nullptr) {
            return nullptr;
        }
        auto event = std::move(events_[raw_pointer]);
        events_.erase(raw_pointer);
        return event;
    }

    void Push(int16_t window, std::unique_ptr<Event> event) {
        auto raw_event_pointer = static_cast<Event*>(event.get());
        events_[raw_event_pointer] = std::move(event);
        events_[raw_event_pointer]->window_idx = window;
        queue_.push(raw_event_pointer);
    }

private:
    bx::SpScUnboundedQueueT<Event> queue_;
    std::map<void*, std::unique_ptr<Event>> events_;
};

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

class Context {
public:
    Context() : glfw_message_queue_(&allocator_), event_queue_(&allocator_) {}

    static Context& Get() {
        static Context instance;
        return instance;
    }

    Context(const Context&) = delete;
    void operator=(const Context&) = delete;

    bool Init() {
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

    int16_t CreateWindow(const WindowParams& params) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        auto window = glfwCreateWindow(
            params.width, params.height, params.title.c_str(), params.monitor, params.share
        );

        glfwSetMouseButtonCallback(window, &Context::MouseButtonCallback);

        int16_t window_idx = static_cast<int16_t>(windows_.size());
        windows_.push_back(GLFWWindowUniquePtr (window));
        window_indices_[window] = window_idx;

        return window_idx;
    }

    void DestroyWindow(int16_t idx) {
        window_indices_.erase(windows_[idx].get());
        windows_[idx].reset();
    }

    GLFWwindow* GetGlfwWindow(int16_t window_idx) {
        return windows_[window_idx].get();
    }

    int16_t GetWindowIdx(GLFWwindow* window) {
        return window_indices_.at(window);
    }

    bx::SpScUnboundedQueueT<GlfwMessage>& GetMessageQueue() {
        return glfw_message_queue_;
    }

    EventQueue& GetEventQueue() {
        return event_queue_;
    }

    bx::Thread* GetDefaultThread() {
        return &default_thread_;
    }

private:
    static bx::DefaultAllocator allocator_;
    bx::SpScUnboundedQueueT<GlfwMessage> glfw_message_queue_;

    EventQueue event_queue_;

    std::vector<GLFWWindowUniquePtr> windows_;
    std::unordered_map<GLFWwindow*, int16_t> window_indices_;

    bx::Thread default_thread_;

    static void GLFWErrorCallback(int error, const char* description) {
        fprintf(stderr, "GLFW error %d: %s\n", error, description);
    }

    static void MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t /* modifiers */) {
        double x_pos, y_pos;
        glfwGetCursorPos(window, &x_pos, &y_pos);

        std::unique_ptr<Event> event(new MouseEvent {
            .x = static_cast<int32_t>(x_pos),
            .y = static_cast<int32_t>(y_pos),
            .button = translateGlfwMouseButton(button),
            .down = action == GLFW_PRESS
        });
        event->type = EventType::Mouse;

        std::cout << "mouse " << (action == GLFW_PRESS ? "down" : "up") << " at " << x_pos << ";" << y_pos << std::endl;

        auto window_idx = Context::Get().GetWindowIdx(window);
        Context::Get().GetEventQueue().Push(window_idx, std::move(event));
    }
};

}