#include "tensor_registry.h"

#include <libshm/libshm.h>
#include <torch/torch.h>

namespace nncc::python {

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

bool TensorControlGui(const string& label, entt::entity tensor_entity, const string& callback_name) {
    auto& context = context::Context::Get();
    auto& registry = context.registry;

    auto& tensor = *registry.get<TensorWithPointer>(tensor_entity);
    bool updated = false;

    if (tensor.ndimension() <= 1) {
        ImGui::Text("%s", label.c_str());

        if (tensor.dtype() == torch::kFloat32) {
            for (auto i = 0; i < tensor.numel(); ++i) {
                auto* element = tensor.data_ptr<float>() + i;
                if (i > 0) {
                    ImGui::SameLine();
                }
                auto control_element = ImGui::VSliderFloat(fmt::format("##{}_dragfloat_{}", label, i).c_str(),
                                                           ImVec2(40, 240), element, -2.0f, 2.0f, "%.2f");
                if (control_element) {
                    updated = true;
                    break;
                }
            }
        }

        if (tensor.dtype() == torch::kInt32) {
            for (auto i = 0; i < tensor.numel(); ++i) {
                auto* element = tensor.data_ptr<int>() + i;
                if (i > 0) {
                    ImGui::SameLine();
                }
                if (tensor.numel() > 1) {
                    ImGui::PushItemWidth(60);
                    if (ImGui::DragInt(fmt::format("##{}_dragint_{}", label, i).c_str(), element)) {
                        updated = true;
                    }
                } else {
                    if (ImGui::InputInt(fmt::format("##{}_inputint_{}", label, i).c_str(), element)) {
                        updated = true;
                    }
                }
                if (updated) {
                    break;
                }
            }
        }

        if (updated) {
            TensorControlEvent event;
            event.tensor_entity = tensor_entity;
            event.callback_name = callback_name;

            context::Context::Get().dispatcher.enqueue(event);
        }
    }

    return updated;
}

void TensorRegistry::OnSharedTensorUpdate(const SharedTensorEvent& event) {
    auto& context = context::Context::Get();
    auto& registry = context.registry;

    entt::entity entity;
    if (!tensors_.contains(event.name)) {
        context.log_message = fmt::format("CPU tensor: {}. {}, {}", event.name, event.manager_handle, event.filename);
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
                              0,
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
    auto reply = do_update.get();
    if (reply.is_null() || reply.as_string() == "true") {
        redis_.set(lock_name.toStdString(), "false");
        redis_.lpush(queue_name.toStdString(), {event.callback_name->toStdString()});
        redis_.sync_commit();
    }
}

void TensorRegistry::Clear() {
    nncc::vector<entt::entity> all_tensors;
    all_tensors.reserve(names_.size());
    std::transform(names_.begin(), names_.end(), std::back_inserter(all_tensors), RetrieveKey());

    for (const auto& entity: all_tensors) {
        nncc::context::Context::Get().registry.destroy(entity);
    }
}

TensorWithPointer::TensorWithPointer(const string& manager_handle,
                                     const string& filename,
                                     torch::Dtype dtype,
                                     const vector<int64_t>& dims) {
    size_t total_bytes = (
            torch::elementSize(dtype)
            * std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<>())
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
}