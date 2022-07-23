#pragma once

#include <bgfx/platform.h>
#include <bx/thread.h>
#include <entt/entt.hpp>

namespace nncc::engine {

using ApplicationLoop = entt::delegate<int()>;

int LoopThreadFunc(bx::Thread* self, void* args);

int Run(ApplicationLoop* loop);

}
