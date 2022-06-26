#include "loop.h"

#include <cpp_redis/cpp_redis>
#include <libshm/libshm.h>

#include <3rdparty/bgfx_imgui/imgui/imgui.h>
#include <nncc/python/shm_communication.h>
#include <nncc/python/tensor_plane.h>

namespace nncc::engine {

class TensorContainer {
public:
    TensorContainer(
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
        tensor_ = torch::from_blob(data_ptr_.get(), at::IntArrayRef(dims.data(), dims.size()));
    }

    const torch::Tensor& operator*() const {
        return tensor_;
    }

    torch::Tensor& operator*() {
        return const_cast<torch::Tensor&>(const_cast<const TensorContainer*>(this)->operator*());
    }

private:
    at::DataPtr data_ptr_;
    torch::Tensor tensor_;
};


int Loop(context::Context* context) {
    auto& window = context->GetWindow(0);

    render::PosNormUVVertex::Init();

    engine::Timer timer;
    engine::Camera camera;
    render::Renderer renderer{};

    bx::FileReader reader;
    const auto fs = nncc::engine::LoadShader(&reader, "fs_default_diffuse");
    const auto vs = nncc::engine::LoadShader(&reader, "vs_default_diffuse");
    auto program = bgfx::createProgram(vs, fs, true);
    context->shader_programs["default_diffuse"] = program;

    cpp_redis::client redis;
    redis.connect();

    imguiCreate();

    redis.set("nncc_update_blend", "true");
    redis.sync_commit();

    bool update_blend = false;

    while (true) {
        if (!engine::ProcessEvents(context)) {
            bgfx::touch(0);
            bgfx::frame();
            break;
        }

        uint8_t imgui_pressed_buttons = 0;
        uint8_t current_button_mask = 1;
        for (size_t i = 0; i < 3; ++i) {
            if (context->mouse_state.buttons[i]) {
                imgui_pressed_buttons |= current_button_mask;
            }
            current_button_mask <<= 1;
        }

        imguiBeginFrame(context->mouse_state.x, context->mouse_state.y, imgui_pressed_buttons, context->mouse_state.z,
                        uint16_t(window.width),
                        uint16_t(window.height)
        );

        if (!ImGui::MouseOverArea()) {
            camera.Update(timer.Timedelta(), context->mouse_state, context->key_state);
        }

        bgfx::touch(0);

        ImGui::SetNextWindowPos(ImVec2(50.0f, 600.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(240.0f, 300.0f), ImGuiCond_FirstUseEver);

        ImGui::Begin("Example: weight blending");

        bool weights_changed = false;
        if (context->weights) {
            auto weight_accessor = context->weights->accessor<float, 1>();
            for (auto i = 0; i < 7; ++i) {
                if (ImGui::InputFloat(fmt::format("weight {}", i + 1).c_str(), &weight_accessor[i], 0.05f, 0.01f)) {
                    weights_changed = true;
                }
            }
        }
        update_blend = weights_changed;

        ImGui::Text("Shared tensors");
        ImGui::BeginListBox("##label");
        ImGui::EndListBox();

        auto submit_blend = redis.get("nncc_update_blend");
        redis.sync_commit();

        if (update_blend && submit_blend.get().as_string() == "true") {
            redis.set("nncc_update_blend", "false");
            redis.lpush("nncc_requests", {"update_blend"});
        }

        redis.sync_commit();

        ImGui::End();
        imguiEndFrame();

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, window.width, window.height);

        renderer.SetViewMatrix(camera.GetViewMatrix());
        renderer.SetProjectionMatrix(30, static_cast<float>(window.width) / static_cast<float>(window.height), 0.01f,
                                     1000.0f);
        renderer.SetViewport({0, 0, static_cast<float>(window.width), static_cast<float>(window.height)});

        const auto& cregistry = context->registry;
        auto view = cregistry.view<render::Material, render::Mesh, engine::Transform>();
        for (auto entity : view) {
            const auto& [material, mesh, transform] = view.get(entity);
            renderer.Prepare(material.shader);
            renderer.Add(mesh, material, transform);
        }

        renderer.Present();

        // TODO: make this a subsystem's job
        context->input_characters.clear();

        bgfx::frame();

        // Time.
        timer.Update();
    }

    imguiDestroy();

    bgfx::destroy(nncc::render::Material::default_texture);

    return 0;
}

int RedisListenerThread(bx::Thread* self, void* args) {
    nncc::python::ListenToRedisSharedTensors(static_cast<nncc::context::Context*>(args));
    return 0;
}

int MainThreadFunc(bx::Thread* self, void* args) {
    using namespace nncc;

    auto context = static_cast<context::Context*>(args);
    auto& window = context->GetWindow(0);

    bgfx::Init init;
    init.resolution.width = (uint32_t) window.width;
    init.resolution.height = (uint32_t) window.height;
    init.resolution.reset = 0;
    if (!bgfx::init(init)) {
        return 1;
    }

    bx::Thread redis_thread;
    redis_thread.init(&RedisListenerThread, context, 0, "redis");

    int result = Loop(context);
    context->Destroy();

    bgfx::shutdown();

    redis_thread.shutdown();
    return result;
}

int Run() {
    using namespace nncc::context;

    auto& context = Context::Get();
    if (!context.Init()) {
        return 1;
    }
    bgfx::renderFrame();

    auto& window = context.GetWindow(0);
    auto glfw_main_window = context.GetGlfwWindow(0);

    auto thread = context.GetDefaultThread();
    thread->init(&MainThreadFunc, &context, 0, "render");

    while (glfw_main_window != nullptr && !glfwWindowShouldClose(glfw_main_window)) {
        glfwWaitEventsTimeout(1. / 120);

        // TODO: support multiple windows
        int width, height;
        glfwGetWindowSize(glfw_main_window, &width, &height);
        if (width != window.width || height != window.height) {
            auto event = std::make_unique<ResizeEvent>();
            event->width = width;
            event->height = height;

            context.GetEventQueue().Push(0, std::move(event));
        }

        GlfwMessage message;
        while (context.GetMessageQueue().read(message)) {
            if (message.type == GlfwMessageType::Destroy) {
                glfwDestroyWindow(glfw_main_window);
                glfw_main_window = nullptr;
            }
        }
        bgfx::renderFrame();
    }

    context.Exit();
    while (bgfx::renderFrame() != bgfx::RenderFrame::NoContext) {}
    thread->shutdown();
    return thread->getExitCode();
}

}

bool nncc::engine::ProcessEvents(nncc::context::Context* context) {
    using namespace nncc::context;

    auto queue = &context->GetEventQueue();
    std::shared_ptr<Event> event;

    do {
        event = queue->Poll();
        if (event == nullptr) {
            break;
        }

        if (event->type == EventType::Exit) {
            return false;

        } else if (event->type == EventType::Resize) {
            auto resize_event = std::dynamic_pointer_cast<ResizeEvent>(event);
            context->SetWindowResolution(event->window_idx, resize_event->width, resize_event->height);
            bgfx::reset(resize_event->width, resize_event->height, 0);
            bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);

        } else if (event->type == EventType::MouseButton) {
            auto btn_event = std::dynamic_pointer_cast<MouseEvent>(event);

            context->mouse_state.x = btn_event->x;
            context->mouse_state.y = btn_event->y;
            context->mouse_state.buttons[static_cast<int>(btn_event->button)] = btn_event->down;

        } else if (event->type == EventType::MouseMove) {
            auto move_event = std::dynamic_pointer_cast<MouseEvent>(event);

            context->mouse_state.x = move_event->x;
            context->mouse_state.y = move_event->y;

        } else if (event->type == EventType::Key) {
            auto key_event = std::dynamic_pointer_cast<KeyEvent>(event);
            context->key_state.modifiers = key_event->modifiers;

            auto key = key_event->key;
            if (key_event->down) {
                context->key_state.pressed_keys.insert(key);
            } else if (context->key_state.pressed_keys.contains(key)) {
                context->key_state.pressed_keys.erase(key);
            }

        } else if (event->type == EventType::Char) {
            auto char_event = std::dynamic_pointer_cast<CharEvent>(event);
            context->input_characters.push_back(char_event->codepoint);

        } else if (event->type == EventType::SharedTensor) {
            auto tensor_event = std::dynamic_pointer_cast<SharedTensorEvent>(event);
            auto& registry = context->registry;

            entt::entity entity;
            if (!context->tensors.contains(tensor_event->name)) {
                entity = registry.create();
                registry.emplace<TensorContainer>(entity, tensor_event->manager_handle, tensor_event->filename, tensor_event->dtype, tensor_event->dims);
                context->tensors[tensor_event->name] = entity;
            } else {
                entity = context->tensors.at(tensor_event->name);
            }

            if (tensor_event->dims.size() == 3) {
                if (!registry.all_of<render::Mesh>(entity)) {
                    registry.emplace<render::Mesh>(entity, render::GetPlaneMesh());
                }

                auto& transform = registry.emplace_or_replace<Transform>(entity);
                Matrix4 x_translation, identity = Matrix4::Identity();
                // TODO: make size correct
                bx::mtxTranslate(*x_translation, 2.1f * context->tensors.size(), 0., 0.);
                bx::mtxMul(*transform, *identity, *x_translation);

                // TODO: texture resource https://github.com/skypjack/entt/wiki/Crash-Course:-resource-management
                render::Material* material;
                if (!registry.all_of<render::Material>(entity)) {
                    auto& _material = registry.emplace<render::Material>(entity);
                    _material.shader = context->shader_programs["default_diffuse"];
                    auto height = tensor_event->dims[0], width = tensor_event->dims[1], channels = tensor_event->dims[2];
                    auto texture_format = GetTextureFormatFromChannelsAndDtype(channels, tensor_event->dtype);
                    _material.diffuse_texture = bgfx::createTexture2D(width, height, false, 0, texture_format, 0);
                    _material.d_texture_uniform = bgfx::createUniform("diffuseTX", bgfx::UniformType::Sampler, 1);
                    _material.d_color_uniform = bgfx::createUniform("diffuseCol", bgfx::UniformType::Vec4, 1);
                    material = &_material;
                } else {
                    material = &registry.get<render::Material>(entity);
                }

                auto& tensor = *registry.get<TensorContainer>(entity);
                auto texture_memory = bgfx::makeRef(tensor.data_ptr(), tensor.storage().nbytes());
                bgfx::updateTexture2D(material->diffuse_texture, 1, 0, 0, 0, tensor.size(1), tensor.size(0), texture_memory);
            }

        } else {
            std::cerr << "unknown event type " << static_cast<int>(event->type) << std::endl;
        }

    } while (event != nullptr);

    return true;
}
