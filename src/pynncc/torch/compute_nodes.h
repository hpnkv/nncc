#pragma once

#include <torch/torch.h>

#include <nncc/nodes/graph.h>

namespace nncc::nodes {

class TensorValue {
public:
    TensorValue(const ComputeValue& value) : TensorValue(As<torch::Tensor>(value.Get())) {}
    struct ExtendedTypeInfo {
        caffe2::TypeMeta dtype_;
    };

    TensorValue() : TensorValue(torch::rand({1, 3, 64, 64})) {}

    TensorValue(const torch::Tensor& tensor) : tensor_(tensor.clone()) {}

    TensorValue(torch::Tensor&& tensor) : tensor_(std::move(tensor)) {}

    ComputeValueType Type() const {
        return ComputeValueType::Tensor;
    }

    std::shared_ptr<void> TypeInfo() const {
        return std::make_shared<ExtendedTypeInfo>(ExtendedTypeInfo{tensor_.dtype()});
    }

    const void* Get() const {
        return static_cast<const void*>(&tensor_);
    }

    TensorValue Negate() const {
        return TensorValue{-tensor_};
    }

    ComputeValue Add(const ComputeValue& other) const {
        if (other.Type() == ComputeValueType::UserDefined) {
            return other.Add(*this);
        } else if (other.Type() == ComputeValueType::Float) {
            return TensorValue(tensor_ + As<float>(other.Get()));
        } else if (other.Type() == ComputeValueType::Int) {
            return TensorValue(tensor_ + As<int>(other.Get()));
        } else if (other.Type() == ComputeValueType::Double) {
            return TensorValue(tensor_ + As<double>(other.Get()));
        } else if (other.Type() == ComputeValueType::Tensor) {
            return TensorValue(tensor_ + As<torch::Tensor>(other.Get()));
        } else if (other.Type() == ComputeValueType::Array) {
            throw std::runtime_error("Tensor-array addition is not implemented.");
        } else {
            throw std::runtime_error("Cannot add these types.");
        }
    }

private:
    torch::Tensor tensor_;
};

}


