#include "context.h"

namespace nncc::context {

static nncc::context::Context context;

Context* Context::Get() {
    return &context;
}

}