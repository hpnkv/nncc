#include "surface.h"

namespace nncc::render {

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

}