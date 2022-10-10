#include <nncc/context/context.h>
#include <nncc/input/input.h>

namespace nncc::input {

void InputSystem::Init() {
    auto& context = *context::Context::Get();

    context.dispatcher.sink<MouseEvent>().connect<&InputSystem::MouseMove>(*this);
    context.dispatcher.sink<MouseEvent>().connect<&InputSystem::MouseButton>(*this);
    context.dispatcher.sink<MouseEvent>().connect<&InputSystem::MouseScroll>(*this);

    context.dispatcher.sink<CharEvent>().connect<&InputSystem::Char>(*this);
    context.dispatcher.sink<KeyEvent>().connect<&InputSystem::Key>(*this);

    context.dispatcher.sink<ResizeEvent>().connect<&InputSystem::Resize>(*this);
}

void InputSystem::Resize(const ResizeEvent& event) {
    auto& context = *context::Context::Get();

    nncc::context::Context::Get()->SetWindowSize(event.window_idx,
                                                 event.width,
                                                 event.height);

    const auto& window = context.GetWindow(event.window_idx);
    bgfx::reset(window.framebuffer_width, window.framebuffer_height, BGFX_RESET_VSYNC | BGFX_RESET_HIDPI);
    bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);
}

}