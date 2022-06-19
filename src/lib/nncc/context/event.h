#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

#include <torch/torch.h>
#include <bx/thread.h>

#include <nncc/context/hid.h>

namespace nncc::context {

enum class EventType {
    Exit,
    MouseButton,
    MouseMove,
    Key,
    Resize,
    Char,
    SharedTensor,

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
    int32_t scroll = 0;

    MouseButton button = MouseButton::None;

    bool down = false;
    bool move = false;
};

struct SharedTensorEvent : public Event {
    explicit SharedTensorEvent(const EventType& _type = EventType::SharedTensor) : Event(_type) {}

    std::string name;
    std::string manager_handle;
    std::string filename;
    torch::Dtype dtype;
    std::vector<int64_t> dims;
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
    explicit EventQueue(bx::AllocatorI* allocator) : queue_(allocator) {}

    std::unique_ptr<Event> Poll();

    void Push(int16_t window, std::unique_ptr<Event> event);

private:
    bx::SpScUnboundedQueueT<Event> queue_;
    std::unordered_map<void*, std::unique_ptr<Event>> events_;
};

}

