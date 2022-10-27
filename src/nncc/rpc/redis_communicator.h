#pragma once

#include <condition_variable>
#include <unordered_map>

#include <bx/thread.h>
#include <cpp_redis/cpp_redis>
#include <folly/String.h>
#include <folly/Range.h>
#include <entt/entt.hpp>

#include <nncc/common/types.h>

namespace nncc::rpc {

const nncc::string kRedisPrefix = "__nncc_";
const nncc::string kRedisQueueName = "queue";

struct RedisMessageEvent {
    nncc::string type;
    nncc::string payload;
};

struct RedisConnectionParameters {
    std::string host = "127.0.0.1";
    size_t port = 6379;
    cpp_redis::connect_callback_t connect_callback = nullptr;
    uint32_t timeout_ms = 0;
    int32_t max_reconnects = 0;
    uint32_t reconnect_interval_ms = 0;
};

int RedisCommunicatorListen(bx::Thread* thread, void* _self);

int RedisCommunicatorListenToSyncRequests(bx::Thread* thread, void* _self);

class RedisCommunicator {
public:
    using DelegateType = entt::delegate<void(const RedisMessageEvent&)>;

    explicit RedisCommunicator(const nncc::string& prefix = kRedisPrefix, const nncc::string& queue = kRedisQueueName);

    void Init(const std::string& host = "127.0.0.1",
              size_t port = 6379,
              const std::shared_ptr<bx::Thread>& thread = nullptr,
              const cpp_redis::connect_callback_t& connect_callback = nullptr,
              uint32_t timeout_ms = 0,
              int32_t max_reconnects = 0,
              uint32_t reconnect_interval_ms = 0);

    void Destroy();

    void Listen();

    void ListenToSyncRequests();

    void RegisterDelegate(const nncc::string& message_type, const DelegateType& delegate);

    void PushMessage(int32_t receiver_id, const nncc::string& type, const nncc::string& payload);

    void PushMessageSync(int32_t receiver_id, const nncc::string& type, const nncc::string& payload);

    void Stop();

    void Sync();

    void OnMessage(const RedisMessageEvent& event);

    void OnStop(const RedisMessageEvent& event);

private:
    static nncc::string GetQueueName(const nncc::string& base, int32_t receiver_id, const nncc::string& prefix = "");

    void ClearNNCCState();

    RedisConnectionParameters params_;

    std::unordered_map<nncc::string, DelegateType> delegates_;
    bool stop_requested_ = false;
    bool initialised_ = false;

    std::mutex sync_mutex_;
    std::condition_variable sync_cv_;
    bool sync_requested_ = false;

    // TODO: clear this mess
    cpp_redis::client sync_client_ {};
    std::shared_ptr<bx::Thread> sync_thread_ = nullptr;

    cpp_redis::client client_ {};
    std::shared_ptr<bx::Thread> thread_ = nullptr;

    const nncc::string prefix_, queue_, queue_base_;
    const int k_hard_timeout_s_ = 10;
};

}