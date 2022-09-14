#include "tensor_registry.h"
#include "shm_communication.h"

#include <folly/String.h>
#include <folly/Range.h>

namespace nncc::python {

int StartSharedTensorRedisLoop(bx::Thread* self, void* dispatcher_) {
    cpp_redis::client redis;
    redis.connect();

    auto dispatcher = static_cast<entt::dispatcher*>(dispatcher_);
    auto queue_name = kRedisQueueName;

    redis.del({queue_name.toStdString()});
    redis.del({"nncc_requests"});
    redis.sync_commit();
    bool done = false;

    while (!done) {
        bool success = true;

        redis.blpop({queue_name.toStdString()}, 0, [&done, &dispatcher, &success](cpp_redis::reply& reply) {
            auto encoded_handle = reply.as_array()[1].as_string();
            if (encoded_handle == kRedisStopString) {
                done = true;
                return;
            }

            nncc::string name, manager_handle, filename, dtype_string, dims_string;
            folly::split("::", encoded_handle, name, manager_handle, filename, dtype_string, dims_string);

            torch::Dtype dtype;
            if (dtype_string == "uint8") {
                dtype = torch::kUInt8;
            } else if (dtype_string == "float32") {
                dtype = torch::kFloat32;
            } else if (dtype_string == "int32") {
                dtype = torch::kInt32;
            } else {
                success = false;
                return;
            }

            nncc::vector<int64_t> dims;
            folly::split(",", dims_string, dims);

            SharedTensorEvent event;
            event.name = name;
            event.manager_handle = manager_handle;
            event.filename = filename;
            event.dtype = dtype;
            event.dims = dims;

            dispatcher->enqueue(event);
        });
        redis.sync_commit();

        if (!success) {
            return 1;
        }
    }

    return 0;
}

void StopSharedTensorRedisLoop(const nncc::string& queue_name) {
    cpp_redis::client redis;
    redis.connect();

    redis.lpush(queue_name.toStdString(), {kRedisStopString.toStdString()});
    redis.sync_commit();
}

}