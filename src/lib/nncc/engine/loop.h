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


nncc::render::Mesh GetPlaneMesh() {
    nncc::render::Mesh mesh;
    mesh.vertices = {
            {{-1.0f, 1.0f,  0.05f},  {0., 0., 1.},  0., 0.},
            {{1.0f,  1.0f,  0.05f},  {0., 0., 1.},  1., 0.},
            {{-1.0f, -1.0f, 0.05f},  {0., 0., 1.},  0., 1.},
            {{1.0f,  -1.0f, 0.05f},  {0., 0., 1.},  1., 1.},
            {{-1.0f, 1.0f,  -0.05f}, {0., 0., -1.}, 0., 0.},
            {{1.0f,  1.0f,  -0.05f}, {0., 0., -1.}, 1., 0.},
            {{-1.0f, -1.0f, -0.05f}, {0., 0., -1.}, 0., 1.},
            {{1.0f,  -1.0f, -0.05f}, {0., 0., -1.}, 1., 1.},
    };
    mesh.indices = {
            0, 2, 1,
            1, 2, 3,
            1, 3, 2,
            0, 1, 2
    };
    return mesh;
}

namespace nncc::engine {

bool ProcessEvents(context::Context* context) {
    auto queue = &context->GetEventQueue();
    std::shared_ptr<context::Event> event = queue->Poll();

    if (event == nullptr) {
        return true;
    }

    if (event->type == context::EventType::Exit) {
        return false;

    } else if (event->type == context::EventType::MouseButton) {
        auto btn_event = std::static_pointer_cast<context::MouseEvent>(event);

        context->mouse_state.x = btn_event->x;
        context->mouse_state.y = btn_event->y;
        context->mouse_state.buttons[static_cast<int>(btn_event->button)] = btn_event->down;

    } else if (event->type == context::EventType::MouseMove) {
        auto move_event = std::static_pointer_cast<context::MouseEvent>(event);

        context->mouse_state.x = move_event->x;
        context->mouse_state.y = move_event->y;

    } else if (event->type == context::EventType::Key) {
        auto key_event = std::static_pointer_cast<context::KeyEvent>(event);
        std::cout << static_cast<int>(key_event->key) << std::endl;

    } else {
        std::cerr << "unknown event type" << std::endl;
    }

    return true;
}

}

int MainThreadFunc(bx::Thread* self, void* args) {
    auto context = static_cast<nncc::context::Context*>(args);
    auto glfw_window = context->GetGlfwWindow(0);

    bgfx::Init init;

    nncc::render::PosNormUVVertex::Init();

    int32_t width, height;
    glfwGetWindowSize(glfw_window, &width, &height);

    init.resolution.width = (uint32_t) width;
    init.resolution.height = (uint32_t) height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        return 1;
    }

    nncc::engine::Camera camera;
    nncc::render::Renderer renderer{};
    auto time_offset = bx::getHPCounter();

    const auto texture_image = nncc::common::LoadImage("texture.png");
    const auto texture = nncc::engine::TextureFromImage(texture_image);

    bx::FileReader reader;
    const auto fs = nncc::engine::LoadShader(&reader, "fs_default_diffuse");
    const auto vs = nncc::engine::LoadShader(&reader, "vs_default_diffuse");
    auto program = bgfx::createProgram(vs, fs, true);

    auto texture_uniform = bgfx::createUniform("diffuseTX", bgfx::UniformType::Sampler, 1);
    auto color_uniform = bgfx::createUniform("diffuseCol", bgfx::UniformType::Vec4, 1);

    auto mesh = GetPlaneMesh();
    nncc::render::Material material{};
    material.diffuse_texture = texture;
    material.diffuse_color = 0xFFFFFF00;
    material.shader = program;
    material.d_texture_uniform = texture_uniform;
    material.d_color_uniform = color_uniform;

    bool exit = false;
    while (!exit) {
        if (!nncc::engine::ProcessEvents(context)) {
            exit = true;
        }

        // Time.
        int64_t now = bx::getHPCounter();
        static int64_t last = now;
        const int64_t frame_time = now - last;
        last = now;
        const double freq = double(bx::getHPFrequency());
        const float time = static_cast<float>(now - time_offset) / double(bx::getHPFrequency());
        const float timedelta = float(frame_time / freq);

        camera.Update(timedelta, context->mouse_state);

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        // Set view 0 default viewport.
        bgfx::setViewRect(0, 0, 0, 1024, 768);

        // This dummy draw call is here to make sure that view 0 is cleared
        // if no other draw calls are submitted to view 0.
        bgfx::touch(0);

        renderer.SetViewMatrix(camera.GetViewMatrix());
        renderer.SetProjectionMatrix(60, width / height, 0.01f, 1000.0f);
        renderer.SetViewport({0, 0, static_cast<float>(width), static_cast<float>(height)});
        renderer.Prepare(program);

        auto transform = nncc::engine::Matrix4::Identity();
        bx::mtxRotateY(*transform, time * 2.3f);

        renderer.Add(mesh, material, transform);
        renderer.Present();

        bgfx::setDebug(BGFX_DEBUG_STATS);

        // Advance to next frame. Rendering thread will be kicked to
        // process submitted rendering primitives.
        bgfx::frame();
    }

    // finalise
    bgfx::destroy(program);
    bgfx::destroy(texture);

    bgfx::destroy(texture_uniform);
    bgfx::destroy(color_uniform);

    if (bgfx::isValid(nncc::render::Material::default_texture)) {
        bgfx::destroy(nncc::render::Material::default_texture);
    }

    bgfx::shutdown();
    return 0;
}

int Run() {
    auto& context = nncc::context::Context::Get();
    if (!context.Init()) {
        return 1;
    }
    bgfx::renderFrame();

    auto window = context.GetGlfwWindow(0);

    auto thread = context.GetDefaultThread();
    thread->init(&MainThreadFunc, &context, 0, "render");

    while (window != nullptr && !glfwWindowShouldClose(window)) {
        glfwWaitEventsTimeout(1. / 120);
        while (auto message = context.GetMessageQueue().pop()) {
            if (message->type == nncc::context::GlfwMessageType::Destroy) {
                glfwDestroyWindow(window);
                window = nullptr;
            }
        }
        bgfx::renderFrame();
    }

    std::unique_ptr<nncc::context::Event> event(new nncc::context::ExitEvent);
    event->type = nncc::context::EventType::Exit;
    context.GetEventQueue().Push(0, std::move(event));

    while (bgfx::RenderFrame::NoContext != bgfx::renderFrame()) {}
    thread->shutdown();
    return thread->getExitCode();
}
