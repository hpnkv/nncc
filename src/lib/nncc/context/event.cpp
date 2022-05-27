#include "event.h"

namespace nncc::context {

std::unique_ptr<Event> EventQueue::Poll() {
    void* raw_pointer = queue_.pop();
    if (raw_pointer == nullptr) {
        return nullptr;
    }
    auto event = std::move(events_[raw_pointer]);
    events_.erase(raw_pointer);
    return event;
}

void EventQueue::Push(int16_t window, std::unique_ptr<Event> event) {
    auto raw_event_pointer = static_cast<Event*>(event.get());
    events_[raw_event_pointer] = std::move(event);
    events_[raw_event_pointer]->window_idx = window;
    queue_.push(raw_event_pointer);
}

}