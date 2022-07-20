#pragma once

#include <cpp_redis/cpp_redis>

#include <nncc/common/types.h>
#include <nncc/context/context.h>

namespace nncc::python {

const nncc::string kRedisQueueName = "nncc_tensors";
const nncc::string kRedisStopString = "::done::";

void ListenToRedisSharedTensors(entt::dispatcher* dispatcher, const nncc::string& queue_name = kRedisQueueName);

void StopCommunicatorThread(const nncc::string& queue_name = kRedisQueueName);

}
