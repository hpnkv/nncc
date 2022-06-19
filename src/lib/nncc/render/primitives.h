#pragma once

#include "./surface.h"

namespace nncc::render {

Mesh GetPlaneMesh() {
    Mesh mesh;
    mesh.vertices = {
            {{-1.0f, 1.0f,  0.05f},  {0., 0., 1.},  0., 0.},
            {{1.0f,  1.0f,  0.05f},  {0., 0., 1.},  1., 0.},
            {{-1.0f, -1.0f, 0.05f},  {0., 0., 1.},  0., 1.},
            {{1.0f,  -1.0f, 0.05f},  {0., 0., 1.},  1., 1.},
            {{-1.0f, 1.0f,  -0.05f}, {0., 0., -1.}, 0., 0.},
            {{1.0f,  1.0f,  -0.05f}, {0., 0., -1.}, 1., 0.},
            {{-1.0f, -1.0f, -0.05f}, {0., 0., -1.}, 0., 1.},
            {{1.0f,  -1.0f, -0.05f}, {0., 0., -1.}, 1., 1.},
    };
    mesh.indices = {
            0, 2, 1,
            1, 2, 3,
            1, 3, 2,
            0, 1, 2
    };

    return mesh;
}

const auto kPlaneMesh = GetPlaneMesh();

}
