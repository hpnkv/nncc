#pragma once

#include <memory>

#include <stb/stb_image.h>

#include <nncc/common/types.h>

namespace nncc::common {

struct Image {
    Image() = default;

    Image(const std::shared_ptr<uint8_t>& image, int width, int height, int channels) :
            buffer(image.get(), image.get() + width * height * channels), width(width), height(height),
            channels(channels) {
    }

    [[nodiscard]] int Size() const {
        return width * height * channels;
    }

    int width = 0, height = 0, channels = 0;
    nncc::vector<uint8_t> buffer;
};


Image LoadImage(const nncc::string& filename);

}

