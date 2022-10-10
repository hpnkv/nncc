#include <nncc/engine/timer.h>

namespace nncc::engine {

Timer::Timer() {
    Init();
}

void Timer::Init() {
    offset_ = previous_ = current_ = bx::getHPCounter();
}

void Timer::Update() {
    previous_ = current_;
    current_ = bx::getHPCounter();
}

float Timer::Timedelta() const {
    auto result = static_cast<float>(current_ - previous_) / static_cast<double>(bx::getHPFrequency());
    return static_cast<float>(result);
}

float Timer::Time() const {
    auto result = static_cast<float>(current_ - offset_) / static_cast<double>(bx::getHPFrequency());
    return static_cast<float>(result);
}

}