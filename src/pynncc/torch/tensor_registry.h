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
#include <nncc/gui/nodes/graph.h>
#include <nncc/rendering/renderer.h>
#include <nncc/rendering/primitives.h>


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
    explicit SharedTensorPicker(TensorRegistry* tensors) : tensors_(*tensors) {}

    void Render() {
        auto& context = context::Context::Get();
        auto& registry = context.registry;
        const auto& cregistry = context.registry;

        const auto& scale_w = context.GetWindow(0).scale_w;
        const auto& scale_h = context.GetWindow(0).scale_h;

        ImGui::SetNextWindowPos(ImVec2(50.0f * scale_w, 50.0f * scale_h), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320.0f * scale_w, 800.0f * scale_h), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Data")) {
            ImGui::LabelText("", "Shared tensors");
            if (ImGui::BeginListBox("##label")) {
                auto named_tensors = cregistry.view<Name, TensorWithPointer>();
                for (auto entity : named_tensors) {
                    const auto& [name, tensor_container] = named_tensors.get(entity);
                    auto is_selected = selected_name_ == name.value;
                    if (ImGui::Selectable(name.value.c_str(), is_selected)) {
                        if (selected_name_ == name.value) {
                            continue;
                        }

                        if (!selected_name_.empty()) {
                            {
                                auto previously_selected_tensor = tensors_.Get(selected_name_);
                                if (registry.try_get<rendering::Material>(previously_selected_tensor)) {
                                    auto& mat = registry.get<rendering::Material>(previously_selected_tensor);
                                    mat.diffuse_color = 0xFFFFFFFF;
                                }
                            }
                        }
                        {
                            auto selected_tensor = tensors_.Get(name.value);
                            if (registry.try_get<rendering::Material>(selected_tensor)) {
                                auto& mat = registry.get<rendering::Material>(selected_tensor);
                                mat.diffuse_color = 0xDDFFDDFF;
                            }
                        }
                        selected_name_ = name.value;
                    }
                }
                ImGui::EndListBox();
            }

            if (!tensors_.Contains(selected_name_)) {
                selected_name_.clear();
            } else {
                auto selected_tensor_entity = tensors_.Get(selected_name_);
                if (registry.all_of<TensorWithPointer, Name, TensorControl>(selected_tensor_entity)) {
                    auto& control_callback = registry.get<TensorControl>(selected_tensor_entity).callback_name;
                    auto& name = registry.get<Name>(selected_tensor_entity).value;
                    ImGui::Text("%s", fmt::format("{}: callback name", name).c_str());
                    ImGui::InputText(fmt::format("##{}_callback_name", name).c_str(), &control_callback);
                }
            }

            auto controllable_tensors = registry.view<TensorWithPointer, Name, TensorControl>(entt::exclude<rendering::Mesh>);
            for (auto&& [entity, tensor, name, control] : controllable_tensors.each()) {
                TensorControlGui(name.value, entity, control.callback_name);
            }

            if (ImGui::SmallButton(ICON_FA_STOP_CIRCLE)) {
                tensors_.Clear();
                selected_name_.clear();
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
    nncc::string selected_name_;
};

//nodes::ComputeNode MakeTensorInputOp(const nncc::string& name) {
//    nodes::ComputeNode node;
//
//    node.name = name;
//    node.type = "TensorInput";
//
//    node.AddSetting(nodes::Attribute("name", nodes::AttributeType::String));
//    node.AddOutput(nodes::Attribute("value", nodes::AttributeType::Tensor));
//
//    auto evaluate_fn = [](nodes::ComputeNode* node, entt::registry* registry) {
//        auto& context = context::Context::Get();
//        auto* tensor_registry = context.subsystems.Get<TensorRegistry>();
//        if (tensor_registry == nullptr) {
//            return nodes::Result{1, "Tensor registry subsystem is not initialised."};
//        }
//
//        tensor_registry->Get();
//
//        node->outputs_by_name.at("value").value = node->settings_by_name.at("value").value;
//        return nodes::Result{0, ""};
//    };
//    node.evaluate.connect<evaluate_fn>();
//
//    auto render_extended_view_fn = [](nodes::ComputeNode* node) {
//        ImGui::InputFloat("Value", &node->settings_by_name.at("value").value, 0.1f, 1.0f, "%0.2f");
//    };
//    node.render_context_ui.connect<render_extended_view_fn>();
//
//    return node;
//}

}

