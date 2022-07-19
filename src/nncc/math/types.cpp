#include "types.h"

#include <cstring>

namespace nncc::math {

Matrix4 Matrix4::Identity() {
    const float data[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    return Matrix4(data);
}

Matrix4::Matrix4(const float* data) {
    std::memcpy(elements_, data, 16 * sizeof(float));
}

const float& Matrix4::At(size_t row, size_t column) const {
    return elements_[4 * row + column];
}

float& Matrix4::At(size_t row, size_t column) {
    return const_cast<float&>(const_cast<const Matrix4*>(this)->At(row, column));
}

float* Matrix4::Data() {
    return elements_;
}

const float* Matrix4::operator*() const {
    return elements_;
}

float* Matrix4::operator*() {
    return const_cast<float*>(const_cast<const Matrix4*>(this)->operator*());
}

Vec3::Vec3(float _x, float _y, float _z) {
    x = _x;
    y = _y;
    z = _z;
}

}