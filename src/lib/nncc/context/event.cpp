#include "event.h"

namespace nncc::context {

std::unique_ptr<Event> EventQueue::Poll() {
    void* raw_pointer = queue_.pop();
    if (raw_pointer == nullptr) {
        return nullptr;
    }
    auto event = std::move(events_.at(raw_pointer));
    events_.erase(raw_pointer);
    return std::move(event);
}

void EventQueue::Push(int16_t window, std::unique_ptr<Event> event) {
    event->window_idx = window;
    auto raw_event_pointer = static_cast<Event*>(event.get());
    events_.insert({raw_event_pointer, std::move(event)});
    queue_.push(raw_event_pointer);
}

}