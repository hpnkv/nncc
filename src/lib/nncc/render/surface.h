#pragma once

#include <bgfx/bgfx.h>

#include <array>
#include <cstdint>
#include <map>
#include <vector>

#include <nncc/common/types.h>
#include <nncc/engine/world_math.h>

namespace nncc::render {

struct Viewport {
    float x, y, width, height;
};


struct PosNormUVVertex {
    engine::Vec3 position;
    engine::Vec3 norm;
    float u = 0, v = 0;

    static void Init();

    static bgfx::VertexLayout layout;
};


template<class T>
using VertexBuffer = nncc::vector<T>;
using VertexBufferIndices = nncc::vector<uint16_t>;

struct Mesh {
public:
    VertexBuffer<PosNormUVVertex> vertices;
    VertexBufferIndices indices;
};


struct Material {
    bgfx::ProgramHandle shader;

    uint32_t diffuse_color = 0xFFFFFFFF;
    bgfx::TextureHandle diffuse_texture = Material::GetDefaultTexture();
    bgfx::UniformHandle d_color_uniform, d_texture_uniform;

    static bgfx::TextureHandle GetDefaultTexture() {
        if (default_texture.idx == bgfx::kInvalidHandle) {
            auto texture_memory = bgfx::makeRef(white.data(), sizeof(uint8_t) * 4);
            Material::default_texture = bgfx::createTexture2D(1, 1, false, 0, bgfx::TextureFormat::RGBA8, 0,
                                                              texture_memory);
        }
        return Material::default_texture;
    }

    static bgfx::TextureHandle default_texture;
    static constexpr std::array<uint8_t, 4> white{0xFF, 0xFF, 0xFF, 0xFF};
};


struct Model {
    nncc::vector<Mesh> meshes;
    nncc::vector<Material> materials;
    std::map<size_t, size_t> mesh_material;
    engine::Transform transform;
};

}