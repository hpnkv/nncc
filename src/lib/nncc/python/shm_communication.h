#pragma once

#include <folly/String.h>

#include <nncc/context/context.h>

namespace nncc::python {

const std::string kRedisQueueName = "nncc_tensors";

void ListenToRedisSharedTensors(context::Context* context, const std::string& queue_name = kRedisQueueName) {
    cpp_redis::client redis;
    redis.connect();
    redis.del({queue_name});
    redis.sync_commit();
    bool done = false;

    while (!done) {
        redis.blpop({queue_name}, 0, [&done, &context] (cpp_redis::reply& reply) {
            auto encoded_handle = reply.as_array()[1].as_string();
            if (encoded_handle == "::done::") {
                done = true;
                return;
            }

            std::string name, manager_handle, filename, dtype_string, dims_string;
            folly::split("::", encoded_handle, name, manager_handle, filename, dtype_string, dims_string);

            torch::Dtype dtype;
            if (dtype_string == "uint8") {
                dtype = torch::kUInt8;
            } else if (dtype_string == "float32") {
                dtype = torch::kFloat32;
            }

            int64_t height, width, channels;
            folly::split(",", dims_string, height, width, channels);

            auto event = std::make_unique<context::SharedTensorEvent>();
            event->name = name;
            event->manager_handle = manager_handle;
            event->filename = filename;
            event->dtype = dtype;
            event->dims = {height, width, channels};
            context->GetEventQueue().Push(0, std::move(event));
        });

        redis.sync_commit();
    }
}

}
