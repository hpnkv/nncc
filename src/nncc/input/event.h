#pragma once

#include <map>
#include <memory>

#include <folly/ProducerConsumerQueue.h>

#include <nncc/common/types.h>
#include <nncc/input/hid.h>

namespace nncc::input {

enum class MouseEventType {
    Button,
    Move,
    Scroll,

    Count,
    None
};


struct MouseEvent {
    int16_t window_idx = 0;
    MouseEventType type = MouseEventType::None;

    int32_t x = 0;
    int32_t y = 0;

    double scroll_x = 0;
    double scroll_y = 0;

    int modifiers = 0;

    MouseButton button = MouseButton::None;

    bool down = false;
    bool move = false;
};


struct KeyEvent {
    int16_t window_idx = 0;

    Key key = Key::None;
    int modifiers = 0;
    bool down = false;
};


struct CharEvent {
    int16_t window_idx = 0;
    unsigned int codepoint = 0;
};


struct ResizeEvent {
    int16_t window_idx = 0;
    int width = 0, height = 0;
};


struct ExitEvent {
    int16_t window_idx = 0;
};

}

