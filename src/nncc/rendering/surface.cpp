#include "surface.h"

namespace nncc::rendering {

bgfx::VertexLayout PosNormUVVertex::layout;

void PosNormUVVertex::Init() {
    layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
}

bgfx::TextureHandle Material::default_texture = BGFX_INVALID_HANDLE;

Mesh GetPlaneMesh() {
    Mesh mesh;
    mesh.vertices = {
        {{-1.0f, 1.0f, 0.05f}, {0., 0., 1.}, 0., 0.},
        {{1.0f, 1.0f, 0.05f}, {0., 0., 1.}, 1., 0.},
        {{-1.0f, -1.0f, 0.05f}, {0., 0., 1.}, 0., 1.},
        {{1.0f, -1.0f, 0.05f}, {0., 0., 1.}, 1., 1.},
        {{-1.0f, 1.0f, -0.05f}, {0., 0., -1.}, 0., 0.},
        {{1.0f, 1.0f, -0.05f}, {0., 0., -1.}, 1., 0.},
        {{-1.0f, -1.0f, -0.05f}, {0., 0., -1.}, 0., 1.},
        {{1.0f, -1.0f, -0.05f}, {0., 0., -1.}, 1., 1.},
    };
    mesh.indices = {
        0, 2, 1,
        1, 2, 3,
        1, 3, 2,
        0, 1, 2
    };

    return mesh;
}

}