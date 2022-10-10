#pragma once

#include <nncc/input/event.h>
#include <nncc/input/hid.h>

namespace nncc::input {

struct InputSystem {
    void Init();

    std::deque<unsigned int> input_characters {};
    KeyState key_state;
    MouseState mouse_state;

    void MouseMove(const MouseEvent& event) {
        if (event.type != MouseEventType::Move) {
            return;
        }
        mouse_state.x = event.x;
        mouse_state.y = event.y;
    }

    void MouseScroll(const MouseEvent& event) {
        if (event.type != MouseEventType::Scroll) {
            return;
        }
        mouse_state.scroll_x += event.scroll_x;
        mouse_state.scroll_y += event.scroll_y;
    }

    void MouseButton(const MouseEvent& event) {
        if (event.type != MouseEventType::Button) {
            return;
        }
        mouse_state.x = event.x;
        mouse_state.y = event.y;
        mouse_state.buttons[static_cast<int>(event.button)] = event.down;
        key_state.modifiers = event.modifiers;
    }

    void Char(const CharEvent& event) {
        input_characters.push_back(event.codepoint);
    }

    void Key(const KeyEvent& event) {
        key_state.modifiers = event.modifiers;

        auto key = event.key;
        if (event.down) {
            key_state.pressed_keys.insert(key);
        } else if (key_state.pressed_keys.contains(key)) {
            key_state.pressed_keys.erase(key);
        }
    }

    void Resize(const ResizeEvent& event);
};

}