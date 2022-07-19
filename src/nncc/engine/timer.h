#include <bx/timer.h>

namespace nncc::engine {

class Timer {
public:
    Timer() {
        Init();
    }

    void Init() {
        offset_ = previous_ = current_ = bx::getHPCounter();
    }

    void Update() {
        previous_ = current_;
        current_ = bx::getHPCounter();
    }

    float Timedelta() const {
        auto result = static_cast<float>(current_ - previous_) / static_cast<double>(bx::getHPFrequency());
        return static_cast<float>(result);
    }

    float Time() const {
        auto result = static_cast<float>(current_ - offset_) / static_cast<double>(bx::getHPFrequency());
        return static_cast<float>(result);
    }

private:
    int64_t offset_ = 0, previous_ = 0, current_ = 0;
};

}