#pragma once

#include <bx/timer.h>

namespace nncc::engine {

class Timer {
public:
    Timer();

    void Init();

    void Update();

    float Timedelta() const;

    float Time() const;

private:
    int64_t offset_ = 0, previous_ = 0, current_ = 0;
};

}