#pragma once

#include <map>
#include <memory>

#include <folly/ProducerConsumerQueue.h>

#include <nncc/common/types.h>
#include <nncc/input/hid.h>

namespace nncc::input {

enum class EventType {
    Exit,
    MouseButton,
    MouseMove,
    MouseScroll,
    Key,
    Resize,
    Char,

    Count,
    None
};


struct Event {
    explicit Event(const EventType& _type = EventType::None) : type(_type) {}

    virtual ~Event() = default;

    int16_t window_idx = 0;
    EventType type;
};


struct MouseEvent : public Event {
    explicit MouseEvent(const EventType& _type = EventType::MouseMove) : Event(_type) {}

    int32_t x = 0;
    int32_t y = 0;

    double scroll_x = 0;
    double scroll_y = 0;

    int modifiers = 0;

    MouseButton button = MouseButton::None;

    bool down = false;
    bool move = false;
};


struct KeyEvent : public Event {
    KeyEvent() : Event(EventType::Key) {}

    Key key = Key::None;
    int modifiers = 0;
    bool down = false;
};


struct CharEvent : public Event {
    CharEvent() : Event(EventType::Char) {}

    unsigned int codepoint = 0;
};


struct ResizeEvent : public Event {
    ResizeEvent() : Event(EventType::Resize) {}

    int width = 0, height = 0;
};


struct ExitEvent : public Event {
    ExitEvent() : Event(EventType::Exit) {}
};


class EventQueue {
public:
    EventQueue() = default;

    std::unique_ptr<Event> Poll();

    void Push(int16_t window, std::unique_ptr<Event> event);

private:
    folly::ProducerConsumerQueue<std::unique_ptr<Event>> queue_{128};
};

}

