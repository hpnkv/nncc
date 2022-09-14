#include "rendering.h"

#include <nncc/context/context.h>

namespace nncc::rendering {

void RenderingSystem::Update(nncc::context::Context& context,
                             const nncc::math::Transform& view_matrix,
                             const nncc::math::Transform& projection_matrix,
                             uint16_t width,
                             uint16_t height) {
    const auto& cregistry = context.registry;

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, width, height);

    renderer_.SetViewMatrix(view_matrix);
    renderer_.SetProjectionMatrix(projection_matrix);
    renderer_.SetViewport({0, 0, static_cast<float>(width), static_cast<float>(height)});

    auto view = cregistry.view<Material, Mesh, math::Transform>();
    for (auto entity : view) {
        const auto& [material, mesh, transform] = view.get(entity);
        renderer_.Prepare(material.shader);
        renderer_.Add(mesh, material, transform);
    }

    renderer_.Present();
}

    int RenderingSystem::Init(uint16_t width, uint16_t height) {
        rendering::PosNormUVVertex::Init();

        auto& context = context::Context::Get();
        auto& window = context.GetWindow(0);

        bgfx::Init init;
        init.platformData.ndt = window.GetNativeDisplayType();
        init.platformData.nwh = window.GetNativeHandle();
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

}


