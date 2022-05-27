#pragma once

#include <cstdint>

namespace nncc::context {

enum class MouseButton {
    None,
    Left,
    Middle,
    Right
};

MouseButton translateGlfwMouseButton(int button);

}