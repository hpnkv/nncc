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
    void Init() {
        bx::FileReader reader;
        const auto fs = nncc::engine::LoadShader(&reader, "fs_default_diffuse");
        const auto vs = nncc::engine::LoadShader(&reader, "vs_default_diffuse");
        auto program = bgfx::createProgram(vs, fs, true);
        shader_programs_["default_diffuse"] = program;
    }

    void Update(context::Context& context, const engine::Transform& view_matrix, uint16_t width, uint16_t height);

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