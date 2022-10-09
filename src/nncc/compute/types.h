#pragma once

#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include <fmt/format.h>
#include <entt/entt.hpp>

#include <nncc/common/types.h>

namespace nncc::compute {

class DataTypeRegistry {
public:
    using CastFnType = entt::delegate<entt::entity(entt::entity, const nncc::string&, const nncc::string&)>;
    using CheckerFnType = entt::delegate<bool(entt::entity)>;

    static DataTypeRegistry* Get();

    DataTypeRegistry() = default;

    // We don't want occasional copies
    DataTypeRegistry(const DataTypeRegistry&) = delete;

    void operator=(const DataTypeRegistry&) = delete;

    void RegisterType(const nncc::string& name) {
        types_.insert(name);
    }

    void RegisterCast(const nncc::string& source_type, const nncc::string& target_type, const CastFnType& cast) {
        if (!types_.contains(source_type)) {
            throw std::runtime_error(fmt::format("Source type \"{}\" is not a registered type.", source_type));
        }
        if (!types_.contains(target_type)) {
            throw std::runtime_error(fmt::format("Target type \"{}\" is not a registered type.", target_type));
        }

        cast_functions_[{source_type, target_type}] = cast;
    }

    void RegisterTypeChecker(const nncc::string& type, const CheckerFnType& checker) {
        if (!types_.contains(type)) {
            throw std::runtime_error(fmt::format("Type \"{}\" is not a registered type.", type));
        }
        checker_functions_[type] = checker;
    }

    bool HasType(entt::entity entity, const nncc::string& type) const {
        return checker_functions_.at(type)(entity);
    }

private:
    std::unordered_set<nncc::string> types_;
    std::unordered_map<std::tuple<nncc::string, nncc::string>, CastFnType> cast_functions_;
    std::unordered_map<nncc::string, CheckerFnType> checker_functions_;
};

struct DataNode {
    template <class TypeStructure>
    static DataNode Create(const nncc::string& type, const TypeStructure& data, entt::registry* registry = nullptr) {
        if (registry == nullptr) {
            registry = &context::Context::Get()->registry;
        }

        auto entity = registry->create();
        registry->emplace<TypeStructure>(entity, data);

        return {type, entity};
    }

    template <class TypeStructure>
    TypeStructure& GetAs(entt::registry* registry = nullptr) {
        if (registry == nullptr) {
            registry = &context::Context::Get()->registry;
        }

        return registry->get<TypeStructure>(entity);
    }

    bool HasType(const nncc::string& _type) const {
        return DataTypeRegistry::Get()->HasType(entity, _type);
    }

    nncc::string type {"NoneType"};
    entt::entity entity {entt::null};
};

struct FloatNode {
    float value;
};

struct StringNode {
    nncc::string value;
};

}