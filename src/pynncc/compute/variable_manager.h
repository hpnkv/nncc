#pragma once

#include <variant>
#include <unordered_map>

#include <nncc/common/types.h>
#include <nncc/rpc/redis_communicator.h>

namespace nncc::python {

using Variable = std::variant<nncc::string, float, int>;

struct VariableUpdateEvent {
    nncc::string name;
    Variable value {};

    nncc::string SerializeFor(int32_t receiver_id) const {
        nncc::string value_string, type;
        if (std::holds_alternative<float>(value)) {
            value_string = folly::to<nncc::string>(std::get<float>(value));
            type = "float";
        } else if (std::holds_alternative<int>(value)) {
            value_string = folly::to<nncc::string>(std::get<int>(value));
            type = "int";
        } else {
            value_string = std::get<nncc::string>(value);
            type = "str";
        }
        nncc::vector<nncc::string> parts = {name, type, value_string, folly::to<nncc::string>(receiver_id)};
        return folly::join("::", parts);
    }
};


class VariableManager {
public:
    VariableManager(rpc::RedisCommunicator* communicator, entt::dispatcher* dispatcher)
      : communicator_(communicator), dispatcher_(dispatcher) {}

    void Init() {
        rpc::RedisCommunicator::DelegateType delegate;
        delegate.connect<&VariableManager::OnVariablePublishMessage>(*this);
        communicator_->RegisterDelegate("const", delegate);
    }

    void OnVariablePublishMessage(const rpc::RedisMessageEvent& event) {
        nncc::string name, type, value;
        int32_t receiver_id;
        folly::split("::", event.payload, name, type, value, receiver_id);

        if (type == "str") {
            vars_[name] = value;
        } else if (type == "float") {
            vars_[name] = folly::to<float>(value);
        } else if (type == "int") {
            vars_[name] = folly::to<int>(value);
        }

        published_vars_.insert({receiver_id, name});
    }

    void AddUpdate(int32_t receiver_id, const nncc::string& name, const Variable& value) {
        if (!published_vars_.contains({receiver_id, name})) {
            return;
        }
        vars_[name] = value;
        updates_by_receiver_[{receiver_id, name}] = {name, value};
        std::cout << "AddUpdate " << updates_by_receiver_.size() << std::endl;
    }

    void PushUpdates() {
        if (updates_by_receiver_.empty()) {
            return;
        }
        for (const auto& [id, update] : updates_by_receiver_) {
            const auto& [receiver_id, name] = id;
            communicator_->PushMessage(receiver_id, "const", update.SerializeFor(receiver_id));
        }
        communicator_->Sync();
        updates_by_receiver_.clear();
        std::cout << "updates pushed" << std::endl;
    }

    template <typename T>
    T& Get(const nncc::string& name, const std::optional<nncc::string>& value = std::nullopt) {
        if (!vars_.contains(name)) {
            vars_[name] = *value;
        }
        return std::get<T>(vars_.at(name));
    }

    template <typename T>
    T& Get(const nncc::string& name, const std::optional<float>& value = std::nullopt) {
        if (!vars_.contains(name)) {
            vars_[name] = *value;
        }
        return std::get<T>(vars_.at(name));
    }

private:
    rpc::RedisCommunicator* communicator_;
    entt::dispatcher* dispatcher_;

    std::unordered_set<std::pair<int32_t, nncc::string>> published_vars_;
    std::unordered_map<std::pair<int32_t, nncc::string>, VariableUpdateEvent> updates_by_receiver_;
    std::unordered_map<nncc::string, Variable> vars_;
};

}