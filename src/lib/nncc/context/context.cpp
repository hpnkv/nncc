#include "context.h"

#include <nncc/python/shm_communication.h>

namespace nncc::context {

bool Context::Init() {
    InitWindowing(&Context::GLFWErrorCallback);

    bgfx::PlatformData pd;
    pd.ndt = windows_[0].GetNativeDisplayType();
    pd.nwh = windows_[0].GetNativeHandle();
    pd.context = nullptr;
    pd.backBuffer = nullptr;
    pd.backBufferDS = nullptr;
    bgfx::setPlatformData(pd);

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

    glfwSetMouseButtonCallback(window.ptr.get(), &Context::MouseButtonCallback);
    glfwSetCursorPosCallback(window.ptr.get(), &Context::CursorPositionCallback);
    glfwSetKeyCallback(window.ptr.get(), &Context::KeyCallback);
    glfwSetCharCallback(window.ptr.get(), &Context::CharacterCallback);

    int16_t window_idx = static_cast<int16_t>(windows_.size());
    window_indices_[window.ptr.get()] = window_idx;
    windows_.push_back(std::move(window));

    return window_idx;
}

void Context::DestroyWindow(int16_t idx) {
    window_indices_.erase(windows_[idx].ptr.get());
    windows_[idx].ptr.reset();
}

void Context::GLFWErrorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

void Context::MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t) {
    double x_pos, y_pos;
    glfwGetCursorPos(window, &x_pos, &y_pos);

    auto event = std::make_unique<input::MouseEvent>(input::EventType::MouseButton);
    event->x = static_cast<int32_t>(x_pos);
    event->y = static_cast<int32_t>(y_pos);
    event->button = input::translateGlfwMouseButton(button);
    event->down = action == GLFW_PRESS;

    auto& context = Context::Get();
    auto window_idx = context.GetWindowIdx(window);
    context.input.queue.Push(window_idx, std::move(event));
}

void Context::CursorPositionCallback(GLFWwindow* window, double x_pos, double y_pos) {
    auto event = std::make_unique<input::MouseEvent>(input::EventType::MouseMove);
    event->x = static_cast<int32_t>(x_pos);
    event->y = static_cast<int32_t>(y_pos);

    auto& context = Context::Get();
    auto window_idx = context.GetWindowIdx(window);
    context.input.queue.Push(window_idx, std::move(event));
}

const auto glfw_key_translation_table = input::GlfwKeyTranslationTable();

void Context::KeyCallback(GLFWwindow* window, int32_t glfw_key, int32_t scancode, int32_t action, int32_t modifiers) {
    if (glfw_key == GLFW_KEY_UNKNOWN) {
        return;
    }

    int mods = input::translateKeyModifiers(modifiers);
    if (!glfw_key_translation_table.contains(glfw_key)) {
        return;
    }
    auto key = glfw_key_translation_table.at(glfw_key);

    auto event = std::make_unique<input::KeyEvent>();
    event->key = key;
    event->modifiers = mods;
    event->down = action == GLFW_PRESS || action == GLFW_REPEAT;

    auto& context = Context::Get();
    auto window_idx = context.GetWindowIdx(window);
    context.input.queue.Push(window_idx, std::move(event));
}

void Context::CharacterCallback(GLFWwindow* window, unsigned int codepoint) {
    auto event = std::make_unique<input::CharEvent>();
    event->codepoint = codepoint;

    auto& context = Context::Get();
    auto window_idx = context.GetWindowIdx(window);
    context.input.queue.Push(window_idx, std::move(event));
}

void Context::Exit() {
    nncc::python::StopCommunicatorThread();
    std::unique_ptr<input::Event> event(new input::ExitEvent);
    Context::Get().input.queue.Push(0, std::move(event));
}

}

bool nncc::input::InputSystem::ProcessEvents() {
    std::shared_ptr<Event> event;

    do {
        event = queue.Poll();
        if (event == nullptr) {
            break;
        }

        if (event->type == EventType::Exit) {
            return true;

        } else if (event->type == EventType::Resize) {
            auto resize_event = std::dynamic_pointer_cast<ResizeEvent>(event);
            nncc::context::Context::Get().SetWindowResolution(event->window_idx, resize_event->width, resize_event->height);
            bgfx::reset(resize_event->width, resize_event->height, 0);
            bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);

        } else if (event->type == EventType::MouseButton) {
            auto btn_event = std::dynamic_pointer_cast<MouseEvent>(event);

            mouse_state.x = btn_event->x;
            mouse_state.y = btn_event->y;
            mouse_state.buttons[static_cast<int>(btn_event->button)] = btn_event->down;

        } else if (event->type == EventType::MouseMove) {
            auto move_event = std::dynamic_pointer_cast<MouseEvent>(event);

            mouse_state.x = move_event->x;
            mouse_state.y = move_event->y;

        } else if (event->type == EventType::Key) {
            auto key_event = std::dynamic_pointer_cast<KeyEvent>(event);
            key_state.modifiers = key_event->modifiers;

            auto key = key_event->key;
            if (key_event->down) {
                key_state.pressed_keys.insert(key);
            } else if (key_state.pressed_keys.contains(key)) {
                key_state.pressed_keys.erase(key);
            }

        } else if (event->type == EventType::Char) {
            auto char_event = std::dynamic_pointer_cast<CharEvent>(event);
            input_characters.push_back(char_event->codepoint);

        } else {
            std::cerr << "unknown event type " << static_cast<int>(event->type) << std::endl;
        }

    } while (event != nullptr);

    return false;
}
