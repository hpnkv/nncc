#include "loop.h"

#include <pybind11/embed.h>
#include <torch/torch.h>
#include <libshm/libshm.h>

#include <3rdparty/bgfx_imgui/imgui/imgui.h>


namespace py = pybind11;


nncc::render::Mesh GetPlaneMesh() {
    nncc::render::Mesh mesh;
    mesh.vertices = {
            {{-1.0f, 1.0f,  0.05f},  {0., 0., 1.},  0., 0.},
            {{1.0f,  1.0f,  0.05f},  {0., 0., 1.},  1., 0.},
            {{-1.0f, -1.0f, 0.05f},  {0., 0., 1.},  0., 1.},
            {{1.0f,  -1.0f, 0.05f},  {0., 0., 1.},  1., 1.},
            {{-1.0f, 1.0f,  -0.05f}, {0., 0., -1.}, 0., 0.},
            {{1.0f,  1.0f,  -0.05f}, {0., 0., -1.}, 1., 0.},
            {{-1.0f, -1.0f, -0.05f}, {0., 0., -1.}, 0., 1.},
            {{1.0f,  -1.0f, -0.05f}, {0., 0., -1.}, 1., 1.},
    };
    mesh.indices = {
            0, 2, 1,
            1, 2, 3,
            1, 3, 2,
            0, 1, 2
    };
    return mesh;
}

const auto kPlaneMesh = GetPlaneMesh();

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

class TensorPlane {
public:
    TensorPlane(
            const std::string& manager_handle,
            const std::string& filename,
            torch::Dtype dtype,
            std::vector<int64_t> dims
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
        tensor_ = torch::from_blob(data_ptr_.get(), torch::ArrayRef(dims));

        // Set up material for this tensor (default shader, custom texture, own uniforms)

        auto& context = context::Context::Get();
        material_.shader = context.shader_programs["default_diffuse"];

        auto height = dims[0], width = dims[1], channels = dims[2];
        auto texture_format = GetTextureFormatFromChannelsAndDtype(channels, dtype);
        material_.diffuse_texture = bgfx::createTexture2D(width, height, false, 0, texture_format, 0);

        material_.d_texture_uniform = bgfx::createUniform("diffuseTX", bgfx::UniformType::Sampler, 1);
        material_.d_color_uniform = bgfx::createUniform("diffuseCol", bgfx::UniformType::Vec4, 1);
    }

    TensorPlane(TensorPlane&& other) noexcept {
        data_ptr_ = std::move(other.data_ptr_);
        tensor_ = std::move(other.tensor_);

        material_ = other.material_;
        transform_ = other.transform_;
    }

    void Update() {
        auto texture_memory = bgfx::makeRef(tensor_.data_ptr(), tensor_.storage().nbytes());
        bgfx::updateTexture2D(material_.diffuse_texture, 1, 0, 0, 0, tensor_.size(1), tensor_.size(0), texture_memory);
    }

    void Render(render::Renderer* renderer, float time) const {
        renderer->Prepare(material_.shader);
        renderer->Add(kPlaneMesh, material_, transform_);
    };

    void SetTransform(const engine::Matrix4& transform) {
        transform_ = transform;
    }

    ~TensorPlane() {
//        if (!resources_moved_) {
//            bgfx::destroy(material_.diffuse_texture);
//            bgfx::destroy(material_.d_texture_uniform);
//            bgfx::destroy(material_.d_color_uniform);
//        }
    }

private:
    at::DataPtr data_ptr_;
    torch::Tensor tensor_;

    render::Material material_;

    engine::Matrix4 transform_ = engine::Matrix4::Identity();
};

}

namespace nncc::engine {

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

    imguiCreate();

    std::unordered_map<std::string, TensorPlane> tensor_planes;

    auto tensor_plane_1 = TensorPlane(
            "/var/folders/5v/0s6hq3gd44z60gsb6nyt01q80000gn/T//torch-shm-dir-aUXF2B/manager.sock",
            "/torch_42854_1773087314_3",
            torch::kU8,
            {512, 512, 4}
    );

    auto tensor_plane_2 = TensorPlane(
            "/var/folders/5v/0s6hq3gd44z60gsb6nyt01q80000gn/T//torch-shm-dir-aUXF2B/manager.sock",
            "/torch_42854_1066532770_5",
            torch::kU8,
            {512, 512, 4}
    );

    auto identity = engine::Matrix4::Identity();

    Matrix4 x_translation;
    bx::mtxTranslate(*x_translation, 2.1, 0., 0.);

    Matrix4 position_2;
    bx::mtxMul(*position_2, *identity, *x_translation);

    tensor_plane_2.SetTransform(position_2);

    tensor_planes.insert({"stylegan", std::move(tensor_plane_1)});
    tensor_planes.insert({"random", std::move(tensor_plane_2)});

    std::string selected_tensor;

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

        if (!ImGui::MouseOverArea()) {
            camera.Update(timer.Timedelta(), context->mouse_state, context->key_state);
        }

        for (auto& [key, tensor_plane] : tensor_planes) {
            tensor_plane.Update();
        }

        bgfx::touch(0);

        imguiBeginFrame(context->mouse_state.x, context->mouse_state.y, imgui_pressed_buttons, context->mouse_state.z,
                        uint16_t(window.width),
                        uint16_t(window.height)
        );

        ImGui::SetNextWindowPos(ImVec2(20.0f, 50.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(240.0f, 300.0f), ImGuiCond_FirstUseEver);

        ImGui::Begin("Example: PyTorch shared memory tensors");

        ImGui::Text("Shared tensors");
        ImGui::BeginListBox("##label");
        for (const auto& [name, tensor_plane] : tensor_planes) {
            auto selected = name == selected_tensor;
            if (ImGui::Selectable(name.c_str(), selected)) {
                selected_tensor = name;
            }
        }
        ImGui::EndListBox();

        if (ImGui::SmallButton(ICON_FA_LINK)) {
//            openUrl(url);
        } else if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Documentation: https://apankov.net");
        }

        ImGui::End();
        imguiEndFrame();

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, window.width, window.height);

        renderer.SetViewMatrix(camera.GetViewMatrix());
        renderer.SetProjectionMatrix(30, static_cast<float>(window.width) / static_cast<float>(window.height), 0.01f,
                                     1000.0f);
        renderer.SetViewport({0, 0, static_cast<float>(window.width), static_cast<float>(window.height)});

        for (const auto& [key, tensor_plane] : tensor_planes) {
            tensor_plane.Render(&renderer, timer.Time());
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

int MainThreadFunc(bx::Thread* self, void* args) {
    using namespace nncc;

    auto context = static_cast<context::Context*>(args);
    auto& window = context->GetWindow(0);

    bgfx::Init init;
    init.resolution.width = (uint32_t) window.width;
    init.resolution.height = (uint32_t) window.height;
    init.resolution.reset = BGFX_RESET_VSYNC;
    if (!bgfx::init(init)) {
        return 1;
    }

    int result = Loop(context);
    context->Destroy();

    bgfx::shutdown();
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

        while (auto message = context.GetMessageQueue().pop()) {
            if (message->type == GlfwMessageType::Destroy) {
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
    std::shared_ptr<Event> event = queue->Poll();

    while (event != nullptr) {
        if (event->type == EventType::Exit) {
            return false;

        } else if (event->type == EventType::Resize) {
            auto resize_event = std::dynamic_pointer_cast<ResizeEvent>(event);
            context->SetWindowResolution(event->window_idx, resize_event->width, resize_event->height);
            bgfx::reset(resize_event->width, resize_event->height, BGFX_RESET_VSYNC);
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

        } else {
            std::cerr << "unknown event type " << static_cast<int>(event->type) << std::endl;
        }

        event = queue->Poll();
    }

    return true;
}
