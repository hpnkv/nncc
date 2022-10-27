#pragma once

#include <bgfx/bgfx.h>
#include <cpp_redis/cpp_redis>
#include <entt/entt.hpp>
#include <torch/torch.h>

#include <nncc/common/types.h>
#include <nncc/rpc/redis_communicator.h>
#include <nncc/engine/camera.h>
#include <nncc/gui/gui.h>


struct TensorControl {
    bool controllable = true;
    nncc::string callback_name{"update_blend"};
};


struct RetrieveKey {
    template<typename T>
    typename T::first_type operator()(T key_and_value) const {
        return key_and_value.first;
    }
};

namespace nncc::python {

bgfx::TextureFormat::Enum GetTextureFormatFromChannelsAndDtype(int64_t channels, const torch::Dtype& dtype);


class TensorWithPointer {
public:
    TensorWithPointer(
            const nncc::string& manager_handle,
            const nncc::string& filename,
            torch::Dtype dtype,
            const nncc::vector<int64_t>& dims
    );

    const torch::Tensor& operator*() const;

    torch::Tensor& operator*();

private:
    at::DataPtr data_ptr_{};
    torch::Tensor tensor_{};
};


struct Name {
    Name() = default;

    explicit Name(const nncc::string& _value);

    nncc::string value;
};


struct TensorControlEvent {
    entt::entity tensor_entity{};
    std::optional<nncc::string> callback_name;
};

struct SharedTensorEvent {
    nncc::string name;
    nncc::string manager_handle;
    nncc::string filename;
    torch::Dtype dtype;
    nncc::vector<int64_t> dims;
};


class TensorRegistry {
public:
    TensorRegistry() = default;

    void Init(entt::dispatcher* dispatcher);

    void OnTensorWithPointerDestroy(entt::registry& registry, entt::entity entity);

    void OnSharedTensorUpdate(const SharedTensorEvent& event);

    void OnSharedTensorControl(const TensorControlEvent& event);

    static void OnSharedTensorMessage(const rpc::RedisMessageEvent& event);

    void Update();

    entt::entity Get(const nncc::string& name);

    bool Contains(const nncc::string& name);

    bool Contains(entt::entity entity);

    void Clear();

private:
    std::unordered_map<nncc::string, entt::entity> tensors_;
    std::unordered_set<entt::entity> drawable_;
    std::unordered_map<entt::entity, nncc::string> names_;

    cpp_redis::client redis_;
};

bool TensorControlGui(const nncc::string& label, entt::entity tensor_entity, const nncc::string& callback_name);

class SharedTensorPicker {
public:
    explicit SharedTensorPicker(TensorRegistry* tensors);

    void Render();

    gui::GuiPiece GetGuiPiece();

private:
    // TODO: remove from here
    nncc::string prompt_;

    TensorRegistry& tensors_;
    entt::entity selected_tensor_ = entt::null;
    engine::Camera* camera_ = nullptr;
};

}

