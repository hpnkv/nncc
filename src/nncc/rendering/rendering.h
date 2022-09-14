#pragma once

#include <unordered_map>

#include <nncc/math/types.h>
#include <nncc/rendering/renderer.h>
#include <nncc/rendering/surface.h>
#include <nncc/rendering/bgfx/loaders.h>

namespace nncc::context {

class Context;

}

namespace nncc::rendering {

class RenderingSystem {
public:
    int Init(uint16_t width, uint16_t height);

    void Update(context::Context& context, const math::Transform& view_matrix, const math::Transform& projection_matrix, uint16_t width, uint16_t height);

    void Destroy() {
        auto default_texture = Material::GetDefaultTexture();
        bgfx::destroy(default_texture);
        for (const auto& [name, handle] : shader_programs_) {
            bgfx::destroy(handle);
        }
    }

    std::unordered_map<nncc::string, bgfx::ProgramHandle> shader_programs_;

private:
    rendering::Renderer renderer_{};
};

}