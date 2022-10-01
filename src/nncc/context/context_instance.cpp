#include "context.h"

namespace nncc::context {



static nncc::context::Context context;

Context* Context::Get() {
//    static Context instance;
    return &context;
}

}

//__attribute__((constructor()))
//void init() {
//}