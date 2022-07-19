#pragma once

#include <nncc/input/event.h>
#include <nncc/input/hid.h>

namespace nncc::input {

struct InputSystem {
    EventQueue queue;

    std::deque<unsigned int> input_characters;
    KeyState key_state;
    MouseState mouse_state;

    bool ProcessEvents();
};

}