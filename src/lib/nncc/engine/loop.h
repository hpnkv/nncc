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

}

int MainThreadFunc(bx::Thread* self, void* args);

int Run();
