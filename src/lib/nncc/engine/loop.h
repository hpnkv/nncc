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

#include <3rdparty/bgfx_imgui/imgui/imgui.h>

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

class ImGuiComponent {
public:
    virtual void Render() = 0;

    virtual ~ImGuiComponent() = default;

private:
};

class TextEdit : public ImGuiComponent {
public:
    TextEdit(const nncc::string& label, const nncc::string& placeholder) : label_(label), content_(placeholder) {}

    void Render() {
        ImGui::InputText(label_.c_str(), &content_);
    }

    const nncc::string& Value() const {
        return content_;
    }

private:
    nncc::string label_, content_;
};

int MainThreadFunc(bx::Thread* self, void* args);

int Run();

}
