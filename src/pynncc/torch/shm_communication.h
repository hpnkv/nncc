#pragma once

#include <cpp_redis/cpp_redis>

#include <nncc/common/types.h>
#include <nncc/context/context.h>

namespace nncc::python {

const nncc::string kRedisQueueName = "nncc_tensors";
const nncc::string kRedisStopString = "::done::";

int StartSharedTensorRedisLoop(bx::Thread* self, void* dispatcher_);

void StopSharedTensorRedisLoop(const nncc::string& queue_name);

}
