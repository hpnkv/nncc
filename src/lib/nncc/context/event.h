#pragma once

#include <map>
#include <memory>

#include <bx/thread.h>

#include <nncc/context/hid.h>

namespace nncc::context {

enum class EventType {
    Exit,
    MouseButton,
    MouseMove,

    Count,
    None
};

struct Event {
    int16_t window_idx = 0;
    EventType type = EventType::None;
};

struct MouseEvent : public Event {
    int32_t x = 0;
    int32_t y = 0;
    int32_t scroll = 0;

    MouseButton button = MouseButton::None;

    bool down = false;
    bool move = false;
};

struct KeyEvent : public Event {
};

struct ExitEvent : public Event {
};

class EventQueue {
public:
    explicit EventQueue(bx::AllocatorI* allocator) : queue_(allocator) {}

    std::unique_ptr<Event> Poll();

    void Push(int16_t window, std::unique_ptr<Event> event);

private:
    bx::SpScUnboundedQueueT<Event> queue_;
    std::map<void*, std::unique_ptr<Event>> events_;
};

}

