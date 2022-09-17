#include "loaders.h"

#include <sstream>

namespace nncc::engine {

const bgfx::Memory* LoadMemory(bx::FileReaderI* reader, const nncc::string& path) {
    if (bx::open(reader, path.c_str())) {
        int size = static_cast<int>(bx::getSize(reader));
        const auto mem = bgfx::alloc(size + 1);
        bx::read(reader, mem->data, size, bx::ErrorAssert{});
        bx::close(reader);
        mem->data[mem->size - 1] = '\0';
        return mem;
    }

    return nullptr;
}

bgfx::ShaderHandle LoadShader(bx::FileReaderI* reader, const nncc::string& name) {
    std::ostringstream shader_path;

    switch (bgfx::getRendererType()) {
        case bgfx::RendererType::Noop:
        case bgfx::RendererType::Direct3D9:
            shader_path << "shaders/dx9/";
            break;
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12:
            shader_path << "shaders/dx11/";
            break;
        case bgfx::RendererType::Agc:
        case bgfx::RendererType::Gnm:
            shader_path << "shaders/pssl/";
            break;
        case bgfx::RendererType::Metal:
            shader_path << "shaders/metal/";
            break;
        case bgfx::RendererType::Nvn:
            shader_path << "shaders/nvn/";
            break;
        case bgfx::RendererType::OpenGL:
            shader_path << "shaders/glsl/";
            break;
        case bgfx::RendererType::OpenGLES:
            shader_path << "shaders/essl/";
            break;
        case bgfx::RendererType::Vulkan:
        case bgfx::RendererType::WebGPU:
            shader_path << "shaders/spirv/";
            break;

        case bgfx::RendererType::Count:
            BX_ASSERT(false, "You should not be here!");
            break;
    }
    shader_path << name << ".bin";
    auto path = shader_path.str();

    bgfx::ShaderHandle handle = bgfx::createShader(LoadMemory(reader, path));
    bgfx::setName(handle, name.c_str());

    return handle;
}

bgfx::TextureHandle TextureFromImage(const nncc::common::Image& image, bool immutable) {
    auto texture_memory = bgfx::makeRef(image.buffer.data(), image.Size());
    nncc::vector<uint8_t> source_texture = image.buffer;
    if (immutable) {
        return bgfx::createTexture2D(image.width, image.height, false, 0, bgfx::TextureFormat::RGB8, 0, texture_memory);
    }

    auto texture = bgfx::createTexture2D(image.width, image.height, false, 0, bgfx::TextureFormat::RGB8, 0);
    bgfx::updateTexture2D(texture, 1, 0, 0, 0, image.width, image.height, texture_memory);
    return texture;
}

}