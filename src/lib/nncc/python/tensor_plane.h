#pragma once

#include <iostream>
#include <numeric>

#include <bgfx/bgfx.h>
#include <torch/torch.h>
#include <libshm/libshm.h>

#include <nncc/common/types.h>
#include <nncc/context/context.h>
#include <nncc/render/renderer.h>
#include <nncc/render/primitives.h>

namespace nncc {

bgfx::TextureFormat::Enum GetTextureFormatFromChannelsAndDtype(int64_t channels, const torch::Dtype& dtype) {
    if (channels == 3 && dtype == torch::kUInt8) {
        return bgfx::TextureFormat::RGB8;
    } else if (channels == 4 && dtype == torch::kFloat32) {
        return bgfx::TextureFormat::RGBA32F;
    } else if (channels == 4 && dtype == torch::kUInt8) {
        return bgfx::TextureFormat::RGBA8;
    } else {
        throw std::runtime_error("Can only visualise uint8 or float32 tensors with 3 or 4 channels (RGB or RGBA).");
    }
}

}

