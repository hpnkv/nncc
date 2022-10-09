#include "graph.h"

namespace nncc::compute {

int Attribute::id_counter = 0;
int ComputeNode::id_counter = 0;

AttributeType AttributeTypeFromString(const string& type) {
    if (type == "Float") {
        return AttributeType::Float;
    } else if (type == "String") {
        return AttributeType::String;
    } else {
        return AttributeType::UserDefined;
    }
}
}