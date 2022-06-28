#include "event.h"

namespace nncc::input {

std::unique_ptr<Event> EventQueue::Poll() {
    std::unique_ptr<Event> front;
    if (!queue_.read(front)) {
        return nullptr;
    }
    return front;
}

void EventQueue::Push(int16_t window, std::unique_ptr<Event> event) {
    queue_.write(std::move(event));
}

}