#pragma once

#include <nncc/compute/graph.h>

namespace nncc::compute {

struct GetSharedTensorOpState {
    nncc::string name;
    entt::entity entity = entt::null;
};

auto GetSharedTensorOpEvaluateFn(ComputeNode* node, entt::registry* registry);

auto GetSharedTensorOpRenderFn(ComputeNode* node);

ComputeNode MakeGetSharedTensorOp(const void* _ = nullptr);

}