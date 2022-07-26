#pragma once

#include <dyno.hpp>

#include <nncc/common/types.h>

using namespace dyno::literals;

namespace nncc::nodes {

struct ComputeValue;

enum class ComputeValueType {
    Float,
    Double,
    Int,
    String,
    Entity,
    Tensor,
    Array,
    PyObject,
    UserDefined,

    Count,
    None
};

struct IComputeValue : decltype(dyno::requires_(
    "copy-construct"_s = dyno::method<ComputeValue(const ComputeValue&)>,
    "Type"_s = dyno::method<ComputeValueType() const>,
    "TypeInfo"_s = dyno::method<std::shared_ptr<void>() const>,
    "Negate"_s = dyno::method<ComputeValue() const>,
    "Add"_s = dyno::method<ComputeValue(const ComputeValue&) const>,
    "Get"_s = dyno::method<const void*() const>,
    "Edit"_s = dyno::method<void*()>
)) {
};

}  // namespace nncc::nodes

template<typename T>
auto const dyno::default_concept_map<nncc::nodes::IComputeValue, T> = dyno::make_concept_map(
    "copy-construct"_s = [](const T& self, const nncc::nodes::ComputeValue& other) { return self.Type(); },
    "Type"_s = [](const T& self) { return self.Type(); },
    "TypeInfo"_s = [](const T& self) { return self.TypeInfo(); },
    "Negate"_s = [](const T& self) { return self.Negate(); },
    "Add"_s = [](const T& self, const nncc::nodes::ComputeValue& other) { return self.Add(other); },
    "Get"_s = [](const T& self) { return self.Get(); },
    "Edit"_s = [](T& self) { return self.Edit(); }
);

namespace nncc::nodes {

struct ComputeValue {
//    template<typename T>
//    ComputeValue(T x) : poly_{x} {}

    template<typename T>
    ComputeValue(const T& x) : poly_{x} {}

    template<>
    ComputeValue(const ComputeValue& x) : poly_{x.poly_} {}

//    template<typename T>
//    ComputeValue& operator=(T&& x) {
//        poly_ = x;
//        return *this;
//    }

    ComputeValueType Type() const { return poly_.virtual_("Type"_s)(); }

    std::shared_ptr<void> TypeInfo() const { return poly_.virtual_("TypeInfo"_s)(); }

    ComputeValue Negate() const { return poly_.virtual_("Negate"_s)(); }

    ComputeValue Add(const ComputeValue& other) const { return poly_.virtual_("Add"_s)(other); }

    const void* Get() const { return poly_.virtual_("Get"_s)(); }

    void* Edit() { return poly_.virtual_("Edit"_s)(); }

private:
    dyno::poly<IComputeValue> poly_;
};


template<typename ValueType>
const ValueType& As(const void* type_erased_value) {
    return *static_cast<const ValueType*>(type_erased_value);
}

template<typename ValueType>
ValueType& As(void* type_erased_value) {
    return *static_cast<ValueType*>(type_erased_value);
}


class FloatValue {
public:
    FloatValue() : FloatValue{0} {}

    FloatValue(const float& value) : value_(value) {}

    FloatValue(const ComputeValue& value) : FloatValue(As<float>(value.Get())) {}

    ComputeValueType Type() const {
        return ComputeValueType::Float;
    }

    std::shared_ptr<void> TypeInfo() const {
        return nullptr;
    }

    FloatValue Negate() const {
        return {-value_};
    }

    const void* Get() const {
        return static_cast<const void*>(&value_);
    }

     void* Edit() {
        return static_cast<void*>(&value_);
    }

    ComputeValue Add(const ComputeValue& other) const {
        if (other.Type() == ComputeValueType::Tensor || other.Type() == ComputeValueType::Array
            || other.Type() == ComputeValueType::Double || other.Type() == ComputeValueType::UserDefined) {
            return other.Add(*this);
        } else if (other.Type() == ComputeValueType::Float) {
            return FloatValue(value_ + As<float>(other.Get()));
        } else if (other.Type() == ComputeValueType::Int) {
            return FloatValue(value_ + As<int>(other.Get()));
        } else {
            throw std::runtime_error("Cannot add these types.");
        }
    }

private:
    float value_;
};

}  // namespace nncc::nodes

