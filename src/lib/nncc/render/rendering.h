#pragma once

#include <unordered_map>

#include <nncc/render/renderer.h>
#include <nncc/render/surface.h>
#include <nncc/render/bgfx/memory.h>
#include <nncc/engine/world_math.h>

namespace nncc::context {

class Context;

}

namespace nncc::render {

class RenderingSystem {
public:
    int Init(uint16_t width, uint16_t height) {
        render::PosNormUVVertex::Init();

        bgfx::Init init;
        init.resolution.width = (uint32_t) width;
        init.resolution.height = (uint32_t) height;
        init.resolution.reset = BGFX_RESET_VSYNC | BGFX_RESET_HIDPI;
        if (!bgfx::init(init)) {
            return 1;
        }

        bx::FileReader reader;
        const auto fs = nncc::engine::LoadShader(&reader, "fs_default_diffuse");
        const auto vs = nncc::engine::LoadShader(&reader, "vs_default_diffuse");
        auto program = bgfx::createProgram(vs, fs, true);
        shader_programs_["default_diffuse"] = program;

        return 0;
    }

    void Update(context::Context& context, const engine::Transform& view_matrix, const engine::Transform& projection_matrix, uint16_t width, uint16_t height);

    void Destroy() {
        auto default_texture = Material::GetDefaultTexture();
        bgfx::destroy(default_texture);
        for (const auto& [name, handle] : shader_programs_) {
            bgfx::destroy(handle);
        }
    }

    std::unordered_map<nncc::string, bgfx::ProgramHandle> shader_programs_;

private:
    render::Renderer renderer_{};
};

}