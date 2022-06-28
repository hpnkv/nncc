#pragma once

#include "./surface.h"

namespace nncc::render {

Mesh GetPlaneMesh();

const auto kPlaneMesh = GetPlaneMesh();

}
