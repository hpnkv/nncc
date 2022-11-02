#include "redis_communicator.h"

#include <iostream>

namespace nncc::rpc {

void RedisCommunicator::Init(const std::string& host,
                             size_t port,
                             const std::shared_ptr<bx::Thread>& thread,
                             const cpp_redis::connect_callback_t& connect_callback,
                             uint32_t timeout_ms,
                             int32_t max_reconnects,
                             uint32_t reconnect_interval_ms) {

    params_ = {host, port, connect_callback, timeout_ms, max_reconnects, reconnect_interval_ms};

    try {
        client_.connect(host, port, connect_callback, timeout_ms, max_reconnects, reconnect_interval_ms);
        sync_client_.connect(host, port, connect_callback, timeout_ms, max_reconnects, reconnect_interval_ms);
        std::cout << "Redis client connected" << std::endl;
        ClearNNCCState();

        thread_ = thread;
        if (thread_ == nullptr) {
            thread_ = std::make_shared<bx::Thread>();
            std::cout << "Thread created" << std::endl;
        }
        sync_thread_ = std::make_shared<bx::Thread>();
        initialised_ = true;

        DelegateType stop_delegate;
        stop_delegate.connect<&RedisCommunicator::OnStop>(this);
        RegisterDelegate("stop", stop_delegate);

        sync_thread_->setThreadName("redis_sync_thread");
        sync_thread_->init(&RedisCommunicatorListenToSyncRequests, this);

        thread_->setThreadName("redis_communicator");
        thread_->init(&RedisCommunicatorListen, this);

    } catch (const std::runtime_error& e) {
        std::cout << e.what() << std::endl;
    }
}

RedisCommunicator::RedisCommunicator(const string& prefix, const string& queue)
    : prefix_(prefix), queue_(GetQueueName(queue, 0, prefix)), queue_base_(fmt::format("{}{}", prefix, queue)) {
}

void RedisCommunicator::Listen() {
    client_.sync_commit();

    std::cout << fmt::format("Started listening to queue {}", queue_) << std::endl;
    while (!stop_requested_) {
        client_.blpop({queue_.toStdString()}, k_hard_timeout_s_, [this](cpp_redis::reply& reply) {
            if (reply.is_null()) {
                std::cout << "blpop: timeout" << std::endl;
                return;
            }

            if (!reply.is_array()) {
                std::cout << "blpop: reply is not an array" << std::endl;
                return;
            }

            nncc::vector<nncc::string> parts;
            folly::split("::", reply.as_array()[1].as_string(), parts);

            nncc::string type = parts[0];
            nncc::string payload = folly::join("::", parts.begin() + 1, parts.end());

            std::cout << fmt::format("{} {}", type, payload) << std::endl;
            std::cout << fmt::format("blpop: message {}", type) << std::endl;
            OnMessage({type, payload});
        });
        client_.sync_commit();
    }
    std::cout << "Stopped listening" << std::endl;
}

void RedisCommunicator::ListenToSyncRequests() {
    while (!stop_requested_) {
        {
            std::unique_lock lock(sync_mutex_);
            sync_cv_.wait(lock, [this] {return sync_requested_ || stop_requested_;});
        }
        if (stop_requested_) {
            break;
        }
        auto queue = fmt::format("{}_{}", queue_base_, 0);
        auto message = fmt::format("{}::{}", "sync", "");

        sync_client_.lpush(queue, {message});
        sync_client_.sync_commit();

        sync_requested_ = false;
    }
}

void RedisCommunicator::RegisterDelegate(const string& message_type, const RedisCommunicator::DelegateType& delegate) {
    std::cout << fmt::format("Registering delegate for message type: {}", message_type) << std::endl;
    delegates_[message_type] = delegate;
}

void RedisCommunicator::OnMessage(const RedisMessageEvent& event) {
    if (delegates_.contains(event.type)) {
        delegates_.at(event.type)(event);
    }
}

nncc::string RedisCommunicator::GetQueueName(const string& base, int32_t receiver_id, const string& prefix) {
    return fmt::format("{}{}_{}", prefix, base, receiver_id);
}

void RedisCommunicator::ClearNNCCState() {
    client_.keys(fmt::format("{}*", prefix_), [this](cpp_redis::reply& reply) {
        std::cout << "Cleaning keys" << std::endl;
        std::vector<std::string> keys;
        for (const auto& key : reply.as_array()) {
            std::cout << key.as_string() << std::endl;
            keys.push_back(key.as_string());
        }
        client_.del(keys);
        client_.commit();
    });
    client_.sync_commit();
}

void RedisCommunicator::Destroy() {
    stop_requested_ = true;
    sync_cv_.notify_one();
}

int RedisCommunicatorListen(bx::Thread* thread, void* _self) {
    auto* self = static_cast<RedisCommunicator*>(_self);
    self->Listen();
    return 0;
}

int RedisCommunicatorListenToSyncRequests(bx::Thread* thread, void* _self) {
    auto* self = static_cast<RedisCommunicator*>(_self);
    self->ListenToSyncRequests();
    return 0;
}

void RedisCommunicator::PushMessage(int32_t receiver_id, const nncc::string& type, const nncc::string& payload) {
    auto queue = fmt::format("{}_{}", queue_base_, receiver_id);
    auto message = fmt::format("{}::{}", type, payload);

    client_.lpush(queue, {message});
    client_.commit();
}

void RedisCommunicator::PushMessageSync(int32_t receiver_id, const nncc::string& type, const nncc::string& payload) {
    PushMessage(receiver_id, type, payload);
    Sync();
}

void RedisCommunicator::OnStop(const RedisMessageEvent& event) {
    stop_requested_ = true;
}

void RedisCommunicator::Stop() {
    auto queue = fmt::format("{}_{}", queue_base_, 0);
    auto message = fmt::format("stop::");

    cpp_redis::client temp_client;
    temp_client.connect(params_.host, params_.port,
                        params_.connect_callback, params_.timeout_ms,
                        params_.max_reconnects, params_.reconnect_interval_ms);

    temp_client.lpush(queue, {message});
    temp_client.sync_commit();
}

void RedisCommunicator::Sync() {
    if (!sync_requested_) {
        sync_requested_ = true;
        sync_cv_.notify_one();
    }
}

}