#include "tensor_registry.h"

#include <numeric>
#include <unordered_set>

#include <fmt/format.h>
#include <libshm/libshm.h>
#include <torch/torch.h>

#include <nncc/context/context.h>
#include <nncc/gui/guizmo.h>
#include <nncc/rendering/primitives.h>
#include <nncc/gui/picking.h>
#include <pynncc/compute/variable_manager.h>

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
    auto& context = *context::Context::Get();
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

            context::Context::Get()->dispatcher.enqueue(event);
        }
    }

    return updated;
}

void TensorRegistry::OnSharedTensorUpdate(const SharedTensorEvent& event) {
    auto& context = *context::Context::Get();
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
        nncc::context::Context::Get()->registry.destroy(entity);
    }
}

void TensorRegistry::Init(entt::dispatcher* dispatcher) {
    auto context = context::Context::Get();
    context->subsystems.Register(this);

    rpc::RedisCommunicator::DelegateType delegate;
    delegate.connect<&TensorRegistry::OnSharedTensorMessage>();
    context->communicator.RegisterDelegate("share_tensor", delegate);

    context->dispatcher.sink<SharedTensorEvent>().connect<&TensorRegistry::OnSharedTensorUpdate>(*this);
    context->dispatcher.sink<TensorControlEvent>().connect<&TensorRegistry::OnSharedTensorControl>(*this);

    redis_.connect();
}

void TensorRegistry::OnTensorWithPointerDestroy(entt::registry& registry, entt::entity entity) {
    tensors_.erase(names_[entity]);
    names_.erase(entity);
    if (drawable_.contains(entity)) {
        drawable_.erase(entity);
    }
}

void TensorRegistry::Update() {
    redis_.commit();
}

entt::entity TensorRegistry::Get(const string& name) {
    if (!tensors_.contains(name)) {
        return entt::null;
    }
    return tensors_.at(name);
}

bool TensorRegistry::Contains(const string& name) {
    return tensors_.contains(name);
}

bool TensorRegistry::Contains(entt::entity entity) {
    return names_.contains(entity);
}

void TensorRegistry::OnSharedTensorMessage(const rpc::RedisMessageEvent& event) {
    nncc::string name, manager_handle, filename, dtype_string, dims_string;
    folly::split("::", event.payload, name, manager_handle, filename, dtype_string, dims_string);

    torch::Dtype dtype;
    if (dtype_string == "uint8") {
        dtype = torch::kUInt8;
    } else if (dtype_string == "float32") {
        dtype = torch::kFloat32;
    } else if (dtype_string == "int32") {
        dtype = torch::kInt32;
    } else {
//                success = false;
        return;
    }

    nncc::vector<int64_t> dims;
    folly::split(",", dims_string, dims);

    SharedTensorEvent share_event;
    share_event.name = name;
    share_event.manager_handle = manager_handle;
    share_event.filename = filename;
    share_event.dtype = dtype;
    share_event.dims = dims;

    auto context = context::Context::Get();
    context->dispatcher.enqueue(share_event);
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

const torch::Tensor& TensorWithPointer::operator*() const {
    return tensor_;
}

torch::Tensor& TensorWithPointer::operator*() {
    return const_cast<torch::Tensor&>(const_cast<const TensorWithPointer*>(this)->operator*());
}

Name::Name(const string& _value) : value(_value) {}

SharedTensorPicker::SharedTensorPicker(TensorRegistry* tensors) : tensors_(*tensors) {
    auto& context = *context::Context::Get();
    camera_ = context.subsystems.Get<engine::Camera>("current_camera");

    if (context.imgui_context != nullptr) {
        ImGui::SetAllocatorFunctions(context.imgui_allocators.p_alloc_func, context.imgui_allocators.p_free_func);
        ImGui::SetCurrentContext(context.imgui_context);
    }
}

void SharedTensorPicker::Render() {
    auto& context = *context::Context::Get();
    auto& registry = context.registry;
    const auto& cregistry = context.registry;

    camera_ = context.subsystems.Get<engine::Camera>("current_camera");

    const auto& scale = context.GetWindow(0).scale;

//    auto object_picker = context.subsystems.Get<gui::ObjectPicker>();
    bgfx::TextureHandle texture{bgfx::kInvalidHandle};

    ImGui::SetNextWindowPos(ImVec2(50.0f * scale, 50.0f * scale), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320.0f * scale, 800.0f * scale), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Data")) {
        ImGui::LabelText("", "Shared tensors");
        if (ImGui::BeginListBox("##label")) {
            auto named_tensors = cregistry.view<Name, TensorWithPointer>();
            entt::entity clicked_tensor = entt::null;

            // if a click in the list is made, save the corresponding entity
            for (auto entity: named_tensors) {
                const auto& [name, tensor_container] = named_tensors.get(entity);
                if (ImGui::Selectable(name.value.c_str(), entity == selected_tensor_)) {
                    clicked_tensor = entity;
//                    object_picker->SetPickedObject(clicked_tensor);
                }
            }

            // if explicit click was made, select corresponding tensor, otherwise select object from picker (not necessarily a tensor)
//            auto new_selected_tensor = (clicked_tensor != entt::null) ? clicked_tensor : object_picker->GetPickedObject();
            auto new_selected_tensor = entt::null;
            // check if that is a tensor, otherwise clear the selection
            if(!named_tensors.contains(new_selected_tensor)) {
                new_selected_tensor = entt::null;
            }

            if (selected_tensor_ != entt::null && new_selected_tensor != selected_tensor_) {
                if (auto material = registry.try_get<rendering::Material>(selected_tensor_)) {
                    material->diffuse_color = 0xFFFFFFFF;
                }
            }

            selected_tensor_ = new_selected_tensor;
            if (selected_tensor_ != entt::null) {
                if (auto material = registry.try_get<rendering::Material>(selected_tensor_)) {
                    material->diffuse_color = 0xDDFFDDFF;
                    texture = material->diffuse_texture;
                }
            }
            ImGui::EndListBox();
        }

        {
            auto vars = context.subsystems.Get<VariableManager>();

            auto& source_class = vars->Get<nncc::string>("source_class", "a photo of a person");
            if (ImGui::InputText("Source class", &source_class)) {
                vars->AddUpdate(1, "source_class", source_class);
            }

            auto& prompt = vars->Get<nncc::string>("prompt", "a photo of an ordinary person");
            if (ImGui::InputText("Target class", &prompt)) {
                vars->AddUpdate(1, "prompt", prompt);
            }
        }

        if (!tensors_.Contains(selected_tensor_)) {
            selected_tensor_ = entt::null;
        } else {
            if (registry.all_of<TensorWithPointer, Name, TensorControl>(selected_tensor_)) {
                auto& control_callback = registry.get<TensorControl>(selected_tensor_).callback_name;
                auto& name = registry.get<Name>(selected_tensor_).value;
                ImGui::Text("%s", fmt::format("{}: callback name", name).c_str());
                ImGui::InputText(fmt::format("##{}_callback_name", name).c_str(), &control_callback);
            }
            if (auto transform = registry.try_get<math::Transform>(selected_tensor_)) {
                if (camera_) {
                    gui::EditWithGuizmo(*camera_->GetViewMatrix(), *camera_->GetProjectionMatrix(), **transform);
                }
            }
        }

        auto controllable_tensors = registry.view<TensorWithPointer, Name, TensorControl>(
            entt::exclude<rendering::Mesh>);
        for (auto&& [entity, tensor, name, control]: controllable_tensors.each()) {
            TensorControlGui(name.value, entity, control.callback_name);
        }

        auto col_width = ImGui::GetColumnWidth();
        if (texture.idx != bgfx::kInvalidHandle) {
            ImGui::Image(texture, ImVec2(320, 160));
        }

        if (ImGui::SmallButton(ICON_FA_STOP_CIRCLE)) {
            tensors_.Clear();
            selected_tensor_ = entt::null;
        } else if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Remove all shared tensors");
        }
        ImGui::End();
    }
}

gui::GuiPiece SharedTensorPicker::GetGuiPiece() {
    gui::GuiPiece tensor_picker_gui;
    tensor_picker_gui.render.connect<&SharedTensorPicker::Render>(this);
    return tensor_picker_gui;
}
}