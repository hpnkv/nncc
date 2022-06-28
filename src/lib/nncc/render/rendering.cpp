#include "rendering.h"

#include <nncc/context/context.h>

namespace nncc::render {

void RenderingSystem::Update(nncc::context::Context& context,
                             const nncc::engine::Transform& view_matrix,
                             uint16_t width,
                             uint16_t height) {
    const auto& cregistry = context.registry;

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, width, height);

    renderer_.SetViewMatrix(view_matrix);
    renderer_.SetProjectionMatrix(30, static_cast<float>(width) / static_cast<float>(height), 0.01f,
                                  1000.0f);
    renderer_.SetViewport({0, 0, static_cast<float>(width), static_cast<float>(height)});

    auto view = cregistry.view<Material, Mesh, engine::Transform>();
    for (auto entity : view) {
        const auto& [material, mesh, transform] = view.get(entity);
        renderer_.Prepare(material.shader);
        renderer_.Add(mesh, material, transform);
    }

    renderer_.Present();
}

}


