#pragma once

#include <bx/file.h>
#include <bgfx/bgfx.h>

#include <string>

#include <nncc/common/types.h>
#include <nncc/common/image.h>

namespace nncc::engine {

const bgfx::Memory* LoadMemory(bx::FileReaderI* reader, const nncc::string& path);

bgfx::ShaderHandle LoadShader(bx::FileReaderI* reader, const nncc::string& name);

bgfx::TextureHandle TextureFromImage(const nncc::common::Image& image, bool immutable = false);

}