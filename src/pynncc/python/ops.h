#pragma once

#include <nncc/gui/nodes/graph.h>

namespace nncc::nodes {

ComputeNode MakeSG2CheckpointOp(const nncc::string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "SG2Checkpoint";

    node.AddSetting(Attribute("value", AttributeType::UserDefined));

    return node;
}

}