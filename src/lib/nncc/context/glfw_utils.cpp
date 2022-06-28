#include "glfw_utils.h"
#include "hid.h"

namespace nncc::input {

MouseButton translateGlfwMouseButton(int button) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        return MouseButton::Left;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        return MouseButton::Right;
    }

    return MouseButton::None;
}

std::unordered_map<int, Key> GlfwKeyTranslationTable() {
    std::unordered_map<int, Key> table;
    table[GLFW_KEY_ESCAPE] = Key::Esc;
    table[GLFW_KEY_ENTER] = Key::Return;
    table[GLFW_KEY_TAB] = Key::Tab;
    table[GLFW_KEY_BACKSPACE] = Key::Backspace;
    table[GLFW_KEY_SPACE] = Key::Space;
    table[GLFW_KEY_UP] = Key::Up;
    table[GLFW_KEY_DOWN] = Key::Down;
    table[GLFW_KEY_LEFT] = Key::Left;
    table[GLFW_KEY_RIGHT] = Key::Right;
    table[GLFW_KEY_PAGE_UP] = Key::PageUp;
    table[GLFW_KEY_PAGE_DOWN] = Key::PageDown;
    table[GLFW_KEY_HOME] = Key::Home;
    table[GLFW_KEY_END] = Key::End;
    table[GLFW_KEY_PRINT_SCREEN] = Key::Print;
    table[GLFW_KEY_KP_ADD] = Key::Plus;
    table[GLFW_KEY_EQUAL] = Key::Plus;
    table[GLFW_KEY_KP_SUBTRACT] = Key::Minus;
    table[GLFW_KEY_MINUS] = Key::Minus;
    table[GLFW_KEY_COMMA] = Key::Comma;
    table[GLFW_KEY_PERIOD] = Key::Period;
    table[GLFW_KEY_SLASH] = Key::Slash;
    table[GLFW_KEY_F1] = Key::F1;
    table[GLFW_KEY_F2] = Key::F2;
    table[GLFW_KEY_F3] = Key::F3;
    table[GLFW_KEY_F4] = Key::F4;
    table[GLFW_KEY_F5] = Key::F5;
    table[GLFW_KEY_F6] = Key::F6;
    table[GLFW_KEY_F7] = Key::F7;
    table[GLFW_KEY_F8] = Key::F8;
    table[GLFW_KEY_F9] = Key::F9;
    table[GLFW_KEY_F10] = Key::F10;
    table[GLFW_KEY_F11] = Key::F11;
    table[GLFW_KEY_F12] = Key::F12;
    table[GLFW_KEY_KP_0] = Key::NumPad0;
    table[GLFW_KEY_KP_1] = Key::NumPad1;
    table[GLFW_KEY_KP_2] = Key::NumPad2;
    table[GLFW_KEY_KP_3] = Key::NumPad3;
    table[GLFW_KEY_KP_4] = Key::NumPad4;
    table[GLFW_KEY_KP_5] = Key::NumPad5;
    table[GLFW_KEY_KP_6] = Key::NumPad6;
    table[GLFW_KEY_KP_7] = Key::NumPad7;
    table[GLFW_KEY_KP_8] = Key::NumPad8;
    table[GLFW_KEY_KP_9] = Key::NumPad9;
    table[GLFW_KEY_0] = Key::Key0;
    table[GLFW_KEY_1] = Key::Key1;
    table[GLFW_KEY_2] = Key::Key2;
    table[GLFW_KEY_3] = Key::Key3;
    table[GLFW_KEY_4] = Key::Key4;
    table[GLFW_KEY_5] = Key::Key5;
    table[GLFW_KEY_6] = Key::Key6;
    table[GLFW_KEY_7] = Key::Key7;
    table[GLFW_KEY_8] = Key::Key8;
    table[GLFW_KEY_9] = Key::Key9;
    table[GLFW_KEY_A] = Key::KeyA;
    table[GLFW_KEY_B] = Key::KeyB;
    table[GLFW_KEY_C] = Key::KeyC;
    table[GLFW_KEY_D] = Key::KeyD;
    table[GLFW_KEY_E] = Key::KeyE;
    table[GLFW_KEY_F] = Key::KeyF;
    table[GLFW_KEY_G] = Key::KeyG;
    table[GLFW_KEY_H] = Key::KeyH;
    table[GLFW_KEY_I] = Key::KeyI;
    table[GLFW_KEY_J] = Key::KeyJ;
    table[GLFW_KEY_K] = Key::KeyK;
    table[GLFW_KEY_L] = Key::KeyL;
    table[GLFW_KEY_M] = Key::KeyM;
    table[GLFW_KEY_N] = Key::KeyN;
    table[GLFW_KEY_O] = Key::KeyO;
    table[GLFW_KEY_P] = Key::KeyP;
    table[GLFW_KEY_Q] = Key::KeyQ;
    table[GLFW_KEY_R] = Key::KeyR;
    table[GLFW_KEY_S] = Key::KeyS;
    table[GLFW_KEY_T] = Key::KeyT;
    table[GLFW_KEY_U] = Key::KeyU;
    table[GLFW_KEY_V] = Key::KeyV;
    table[GLFW_KEY_W] = Key::KeyW;
    table[GLFW_KEY_X] = Key::KeyX;
    table[GLFW_KEY_Y] = Key::KeyY;
    table[GLFW_KEY_Z] = Key::KeyZ;
    return table;
}

uint8_t translateKeyModifiers(int glfw_modifiers) {
    uint8_t modifiers = 0;

    if (glfw_modifiers & GLFW_MOD_ALT) {
        modifiers |= Modifier::LeftAlt;
    }

    if (glfw_modifiers & GLFW_MOD_CONTROL) {
        modifiers |= Modifier::LeftCtrl;
    }

    if (glfw_modifiers & GLFW_MOD_SUPER) {
        modifiers |= Modifier::LeftMeta;
    }

    if (glfw_modifiers & GLFW_MOD_SHIFT) {
        modifiers |= Modifier::LeftShift;
    }

    return modifiers;
}

}