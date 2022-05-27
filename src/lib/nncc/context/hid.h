#pragma once

#include <cstdint>

namespace nncc::context {

enum class MouseButton {
    Left,
    Middle,
    Right,

    Count,
    None
};

struct MouseState {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    bool buttons[static_cast<int>(MouseButton::Count)] {};
};

MouseButton translateGlfwMouseButton(int button);

}