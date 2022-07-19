#include "tensor_registry.h"

namespace nncc {

bgfx::TextureFormat::Enum GetTextureFormatFromChannelsAndDtype(int64_t channels, const torch::Dtype& dtype) {
    if (channels == 3 && dtype == torch::kUInt8) {
        return bgfx::TextureFormat::RGB8;
    } else if (channels == 4 && dtype == torch::kFloat32) {
        return bgfx::TextureFormat::RGBA32F;
    } else if (channels == 4 && dtype == torch::kUInt8) {
        return bgfx::TextureFormat::RGBA8;
    } else {
        throw std::runtime_error("Can only visualise uint8 or float32 tensors with 3 or 4 channels (RGB or RGBA).");
    }
}

void TensorRegistry::OnSharedTensorUpdate(const SharedTensorEvent& event) {
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
        if (!registry.all_of<rendering::Mesh>(entity)) {
            registry.emplace<rendering::Mesh>(entity, rendering::GetPlaneMesh());

            auto& transform = registry.emplace_or_replace<math::Transform>(entity);
            math::Matrix4 x_translation, identity = math::Matrix4::Identity();
            bx::mtxTranslate(*x_translation, 2.1f * drawable_.size(), 0., 0.);
            bx::mtxMul(*transform, *identity, *x_translation);

            auto scale = identity;
            bx::mtxScale(*scale, static_cast<float>(event.dims[1]) / static_cast<float>(event.dims[0]), 1.0f, 1.0f);
            bx::mtxMul(*transform, *transform, *scale);
        }

        // TODO: texture resource https://github.com/skypjack/entt/wiki/Crash-Course:-resource-management
        rendering::Material* material;
        if (!registry.all_of<rendering::Material>(entity)) {
            auto& _material = registry.emplace<rendering::Material>(entity);
            _material.shader = context.rendering.shader_programs_["default_diffuse"];
            auto height = event.dims[0], width = event.dims[1], channels = event.dims[2];
            auto texture_format = GetTextureFormatFromChannelsAndDtype(channels, event.dtype);
            _material.diffuse_texture = bgfx::createTexture2D(width, height, false, 0, texture_format, 0);
            _material.d_texture_uniform = bgfx::createUniform("diffuseTX", bgfx::UniformType::Sampler, 1);
            _material.d_color_uniform = bgfx::createUniform("diffuseCol", bgfx::UniformType::Vec4, 1);
            material = &_material;

        } else {
            material = &registry.get<rendering::Material>(entity);
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

void TensorRegistry::OnSharedTensorControl(const TensorControlEvent& event) {
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

void TensorRegistry::Clear() {
    nncc::vector<entt::entity> all_tensors;
    all_tensors.reserve(names_.size());
    std::transform(names_.begin(), names_.end(), std::back_inserter(all_tensors), RetrieveKey());

    for (const auto& entity : all_tensors) {
        nncc::context::Context::Get().registry.destroy(entity);
    }
}
}