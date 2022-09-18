#pragma once

#include <iostream>
#include <numeric>
#include <unordered_set>

#include <torch/torch.h>
#include <bgfx/bgfx.h>
#include <cpp_redis/cpp_redis>
#include <fmt/format.h>

#include <nncc/common/types.h>
#include <nncc/context/context.h>
#include <nncc/gui/gui.h>
#include <nncc/gui/guizmo.h>
#include <nncc/gui/nodes/graph.h>
#include <nncc/rendering/renderer.h>
#include <nncc/rendering/primitives.h>
#include <nncc/engine/camera.h>
#include "nncc/gui/picking.h"


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

    const torch::Tensor& operator*() const {
        return tensor_;
    }

    torch::Tensor& operator*() {
        return const_cast<torch::Tensor&>(const_cast<const TensorWithPointer*>(this)->operator*());
    }

private:
    at::DataPtr data_ptr_{};
    torch::Tensor tensor_{};
};


struct Name {
    Name() = default;

    explicit Name(const nncc::string& _value) : value(_value) {}

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

    void Init(entt::dispatcher* dispatcher) {
        context::Context::Get().subsystems.Register(this);
        dispatcher->sink<SharedTensorEvent>().connect<&TensorRegistry::OnSharedTensorUpdate>(*this);
        dispatcher->sink<TensorControlEvent>().connect<&TensorRegistry::OnSharedTensorControl>(*this);
        redis_.connect();
    }

    void OnTensorWithPointerDestroy(entt::registry& registry, entt::entity entity) {
        tensors_.erase(names_[entity]);
        names_.erase(entity);
        if (drawable_.contains(entity)) {
            drawable_.erase(entity);
        }
    }

    void OnSharedTensorUpdate(const SharedTensorEvent& event);

    void OnSharedTensorControl(const TensorControlEvent& event);

    void Update() {
        redis_.commit();
    }

    entt::entity Get(const nncc::string& name) {
        return tensors_.at(name);
    }

    bool Contains(const nncc::string& name) {
        return tensors_.contains(name);
    }

    bool Contains(entt::entity entity) {
        return names_.contains(entity);
    }

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
    explicit SharedTensorPicker(TensorRegistry* tensors) : tensors_(*tensors) {
        auto& context = context::Context::Get();
        camera_ = context.subsystems.Get<engine::Camera>("current_camera");
    }

    void Render() {
        auto& context = context::Context::Get();
        auto& registry = context.registry;
        const auto& cregistry = context.registry;

        camera_ = context.subsystems.Get<engine::Camera>("current_camera");

        const auto& scale = context.GetWindow(0).scale;

        auto object_picker = context.subsystems.Get<gui::ObjectPicker>();

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
                        object_picker->SetPickedObject(clicked_tensor);
                    }
                }

                // if explicit click was made, select corresponding tensor, otherwise select object from picker (not necessarily a tensor)
                auto new_selected_tensor = (clicked_tensor != entt::null) ? clicked_tensor : object_picker->GetPickedObject();
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
                    }
                }

                ImGui::EndListBox();
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

            if (ImGui::SmallButton(ICON_FA_STOP_CIRCLE)) {
                tensors_.Clear();
                selected_tensor_ = entt::null;
            } else if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Remove all shared tensors");
            }
            ImGui::End();
        }
    }

    gui::GuiPiece GetGuiPiece() {
        gui::GuiPiece tensor_picker_gui;
        tensor_picker_gui.render.connect<&SharedTensorPicker::Render>(this);
        return tensor_picker_gui;
    }

private:
    TensorRegistry& tensors_;
    entt::entity selected_tensor_ = entt::null;
    engine::Camera* camera_ = nullptr;
};

}

