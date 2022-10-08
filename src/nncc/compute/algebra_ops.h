#pragma once

#include <nncc/compute/graph.h>

namespace nncc::compute {

ComputeNode MakeConstOp(const void* _ = nullptr);

ComputeNode MakeAddOp(const void* _ = nullptr);

ComputeNode MakeMulOp(const void* _ = nullptr);

}