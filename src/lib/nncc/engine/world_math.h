#pragma once

#include <array>
#include <vector>

namespace nncc::engine {

class Matrix4 {
public:
    Matrix4() {}

    static Matrix4 Identity() {
        const float data[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
        return Matrix4(data);
    }

    explicit Matrix4(const float* data) {
        std::memcpy(elements_, data, 16 * sizeof(float));
    }

    const float& At(size_t row, size_t column) const {
        return elements_[4 * row + column];
    }

    float& At(size_t row, size_t column) {
        return const_cast<float&>(const_cast<const Matrix4*>(this)->At(row, column));
    }

    float* Data() {
        return elements_;
    }

    const float* operator*() const {
        return elements_;
    }

    float* operator*() {
        return const_cast<float*>(const_cast<const Matrix4*>(this)->operator*());
    }

private:
    float elements_[16] {};
};


struct Vec3 {
    Vec3() = default;
    Vec3(float _x, float _y, float _z) {
        x = _x;
        y = _y;
        z = _z;
    }
    float x = 0, y = 0, z = 0;
};

}