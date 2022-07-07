#pragma once

#include <iostream>
#include <numeric>
#include <unordered_set>

#include <cpp_redis/cpp_redis>
#include <bgfx/bgfx.h>
#include <torch/torch.h>
#include <libshm/libshm.h>

#include <nncc/common/types.h>
#include <nncc/context/context.h>
#include <nncc/render/renderer.h>
#include <nncc/render/primitives.h>


struct TensorControl {
    bool controllable = true;
    nncc::string callback_name;
};


struct RetrieveKey {
    template<typename T>
    typename T::first_type operator()(T key_and_value) const {
        return key_and_value.first;
    }
};

namespace nncc {

bgfx::TextureFormat::Enum GetTextureFormatFromChannelsAndDtype(int64_t channels, const torch::Dtype& dtype);


class TensorWithPointer {
public:
    TensorWithPointer(
        const nncc::string& manager_handle,
        const nncc::string& filename,
        torch::Dtype dtype,
        const nncc::vector<int64_t>& dims
    ) {
        size_t total_bytes = (
            torch::elementSize(dtype)
                * std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<size_t>())
        );

        data_ptr_ = THManagedMapAllocator::makeDataPtr(
            manager_handle.c_str(),
            filename.c_str(),
            at::ALLOCATOR_MAPPED_SHAREDMEM,
            total_bytes
        );
        tensor_ = torch::from_blob(data_ptr_.get(),
                                   at::IntArrayRef(dims.data(), dims.size()),
                                   torch::TensorOptions().dtype(dtype));
    }

    const torch::Tensor& operator*() const {
        return tensor_;
    }

    torch::Tensor& operator*() {
        return const_cast<torch::Tensor&>(const_cast<const TensorWithPointer*>(this)->operator*());
    }

private:
    at::DataPtr data_ptr_;
    torch::Tensor tensor_;
};


struct Name {
    Name() = default;

    Name(const nncc::string& _value) : value(_value) {}

    nncc::string value;
};


struct TensorControlEvent {
    entt::entity tensor_entity;
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

    void Init(entt::dispatcher* dispatcher) {
        dispatcher->sink<SharedTensorEvent>().connect<&TensorRegistry::OnSharedTensorUpdate>(*this);
        dispatcher->sink<TensorControlEvent>().connect<&TensorRegistry::OnSharedTensorControl>(*this);
        redis_.connect();
    }

    void OnTensorWithPointerDestroy(entt::registry& registry, entt::entity entity) {
        tensors_.erase(names_[entity]);
        names_.erase(entity);
        control_request_in_progress_.erase(entity);
        if (drawable_.contains(entity)) {
            drawable_.erase(entity);
        }
    }

    void OnSharedTensorUpdate(const SharedTensorEvent& event) {
        auto& context = context::Context::Get();
        auto& registry = context.registry;

        entt::entity entity;
        if (!tensors_.contains(event.name)) {
            entity = registry.create();
            registry.emplace<TensorWithPointer>(entity, event.manager_handle, event.filename, event.dtype, event.dims);
            registry.emplace<Name>(entity, event.name);

            tensors_[event.name] = entity;
            names_[entity] = event.name;

            registry.on_destroy<TensorWithPointer>().connect<&TensorRegistry::OnTensorWithPointerDestroy>(*this);

        } else {
            entity = tensors_.at(event.name);
        }

        if (event.dims.size() == 3) {
            if (!registry.all_of<render::Mesh>(entity)) {
                registry.emplace<render::Mesh>(entity, render::GetPlaneMesh());

                auto& transform = registry.emplace_or_replace<engine::Transform>(entity);
                engine::Matrix4 x_translation, identity = engine::Matrix4::Identity();
                bx::mtxTranslate(*x_translation, 2.1f * drawable_.size(), 0., 0.);
                bx::mtxMul(*transform, *identity, *x_translation);

                auto scale = identity;
                bx::mtxScale(*scale, static_cast<float>(event.dims[1]) / static_cast<float>(event.dims[0]), 1.0f, 1.0f);
                bx::mtxMul(*transform, *transform, *scale);
            }

            // TODO: texture resource https://github.com/skypjack/entt/wiki/Crash-Course:-resource-management
            render::Material* material;
            if (!registry.all_of<render::Material>(entity)) {
                auto& _material = registry.emplace<render::Material>(entity);
                _material.shader = context.rendering.shader_programs_["default_diffuse"];
                auto height = event.dims[0], width = event.dims[1], channels = event.dims[2];
                auto texture_format = GetTextureFormatFromChannelsAndDtype(channels, event.dtype);
                _material.diffuse_texture = bgfx::createTexture2D(width, height, false, 0, texture_format, 0);
                _material.d_texture_uniform = bgfx::createUniform("diffuseTX", bgfx::UniformType::Sampler, 1);
                _material.d_color_uniform = bgfx::createUniform("diffuseCol", bgfx::UniformType::Vec4, 1);
                material = &_material;

            } else {
                material = &registry.get<render::Material>(entity);
            }

            auto& tensor = *registry.get<TensorWithPointer>(entity);
            auto texture_memory = bgfx::makeRef(tensor.data_ptr(), tensor.storage().nbytes());
            bgfx::updateTexture2D(material->diffuse_texture,
                                  1,
                                  0,
                                  0,
                                  0,
                                  tensor.size(1),
                                  tensor.size(0),
                                  texture_memory);

            drawable_.insert(entity);

        } else if (event.dims.empty() || event.dims.size() == 1) {
            if (!registry.all_of<TensorControl>(entity)) {
                registry.emplace<TensorControl>(entity);
            }
        }
    }

    void OnSharedTensorControl(const TensorControlEvent& event) {
        if (!event.callback_name) {
            return;
        }

        nncc::string queue_name = "nncc_requests";
        nncc::string lock_name = fmt::format("nncc_{}", *event.callback_name);

        auto do_update = redis_.get(lock_name.toStdString());
        redis_.sync_commit();
        if (do_update.get().as_string() == "true") {
            redis_.set(lock_name.toStdString(), "false");
            redis_.lpush(queue_name.toStdString(), {event.callback_name->toStdString()});
            redis_.sync_commit();
        }
    }

    void Update() {
        redis_.commit();
    }

    entt::entity Get(const nncc::string& name) {
        return tensors_.at(name);
    }

    bool Contains(const nncc::string& name) {
        return tensors_.contains(name);
    }

    void Clear() {
        nncc::vector<entt::entity> all_tensors;
        all_tensors.reserve(names_.size());
        std::transform(names_.begin(), names_.end(), std::back_inserter(all_tensors), RetrieveKey());

        for (const auto& entity : all_tensors) {
            nncc::context::Context::Get().registry.destroy(entity);
        }
    }

private:
    std::unordered_map<nncc::string, entt::entity> tensors_;
    std::unordered_set<entt::entity> drawable_;
    std::unordered_map<entt::entity, nncc::string> names_;
    std::unordered_map<entt::entity, bool> control_request_in_progress_;

    cpp_redis::client redis_;
};

}

