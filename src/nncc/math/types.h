#pragma once

#include <cstddef>

namespace nncc::math {

class Matrix4 {
public:
    Matrix4() = default;

    static Matrix4 Identity();

    explicit Matrix4(const float* data);

    [[nodiscard]] const float& At(size_t row, size_t column) const;

    float& At(size_t row, size_t column);

    float* Data();

    const float* operator*() const;

    float* operator*();

private:
    float elements_[16]{};
};


struct Vec3 {
    Vec3() = default;

    Vec3(float _x, float _y, float _z);

    float x = 0, y = 0, z = 0;
};


using Transform = Matrix4;
using Position = Vec3;
using Direction = Vec3;

}