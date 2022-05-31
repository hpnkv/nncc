#pragma once

#include <iostream>

#include <bx/thread.h>
#include <bx/timer.h>
#include <bgfx/platform.h>

#include <nncc/common/image.h>
#include <nncc/context/context.h>

#include <nncc/engine/camera.h>

#include <nncc/render/renderer.h>
#include <nncc/render/surface.h>
#include <nncc/render/bgfx/memory.h>


nncc::render::Mesh GetPlaneMesh();

namespace nncc::engine {

bool ProcessEvents(context::Context* context);

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

int MainThreadFunc(bx::Thread* self, void* args);

int Run();
