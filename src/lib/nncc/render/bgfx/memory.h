#pragma once

#include <bx/file.h>
#include <bgfx/bgfx.h>

#include <string>

#include <nncc/common/image.h>

namespace nncc::engine {

const bgfx::Memory* LoadMemory(bx::FileReaderI* reader, const std::string& path);

bgfx::ShaderHandle LoadShader(bx::FileReaderI* reader, const std::string& name);

bgfx::TextureHandle TextureFromImage(const nncc::common::Image& image, bool immutable = false);

}