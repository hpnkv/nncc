#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace nncc::common {

Image LoadImage(const nncc::string& filename) {
    int width, height, channels;
    std::shared_ptr<uint8_t> image(stbi_load(filename.c_str(), &width, &height, &channels, 0));
    return {image, width, height, channels};
}

}