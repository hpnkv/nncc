#pragma once

#include "picking.h"

namespace nncc::gui {


bgfx::ProgramHandle LoadProgram(const string& vs_name, const string& fs_name) {
    bx::FileReader reader;
    const auto fs = nncc::engine::LoadShader(&reader, fs_name);
    const auto vs = nncc::engine::LoadShader(&reader, vs_name);
    return bgfx::createProgram(vs, fs, true);
}
}