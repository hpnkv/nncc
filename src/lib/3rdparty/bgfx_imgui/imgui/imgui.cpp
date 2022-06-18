#include <bgfx/bgfx.h>
#include <bx/allocator.h>
#include <bx/math.h>
#include <bx/timer.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <nncc/render/bgfx/memory.h>
#include <nncc/context/context.h>

#include "imgui.h"

#include "./fonts/roboto_regular.ttf.h"
#include "./fonts/robotomono_regular.ttf.h"
#include "./fonts/icons_kenney.ttf.h"
#include "./fonts/icons_font_awesome.ttf.h"

/// Returns true if both internal transient index and vertex buffer have
/// enough space.
///
/// @param[in] _numVertices Number of vertices.
/// @param[in] _layout Vertex layout.
/// @param[in] _numIndices Number of indices.
///
inline bool checkAvailTransientBuffers(uint32_t _numVertices, const bgfx::VertexLayout& _layout, uint32_t _numIndices) {
    return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _layout)
           && (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices));
}

std::unordered_map<nncc::context::Key, ImGuiKey_> MakeKeyTranslationTable() {
    using namespace nncc::context;

    std::unordered_map<Key, ImGuiKey_> map;
    map[Key::Esc] = ImGuiKey_Escape;
    map[Key::Return] = ImGuiKey_Enter;
    map[Key::Tab] = ImGuiKey_Tab;
    map[Key::Space] = ImGuiKey_Space;
    map[Key::Backspace] = ImGuiKey_Backspace;
    map[Key::Up] = ImGuiKey_UpArrow;
    map[Key::Down] = ImGuiKey_DownArrow;
    map[Key::Left] = ImGuiKey_LeftArrow;
    map[Key::Right] = ImGuiKey_RightArrow;
    map[Key::Insert] = ImGuiKey_Insert;
    map[Key::Delete] = ImGuiKey_Delete;
    map[Key::Home] = ImGuiKey_Home;
    map[Key::End] = ImGuiKey_End;
    map[Key::PageUp] = ImGuiKey_PageUp;
    map[Key::PageDown] = ImGuiKey_PageDown;
    map[Key::Print] = ImGuiKey_PrintScreen;
    map[Key::Plus] = ImGuiKey_Equal;
    map[Key::Minus] = ImGuiKey_Minus;
    map[Key::LeftBracket] = ImGuiKey_LeftBracket;
    map[Key::RightBracket] = ImGuiKey_RightBracket;
    map[Key::Semicolon] = ImGuiKey_Semicolon;
    map[Key::Quote] = ImGuiKey_Apostrophe;
    map[Key::Comma] = ImGuiKey_Comma;
    map[Key::Period] = ImGuiKey_Period;
    map[Key::Slash] = ImGuiKey_Slash;
    map[Key::Backslash] = ImGuiKey_Backslash;
    map[Key::Tilde] = ImGuiKey_GraveAccent;
    map[Key::F1] = ImGuiKey_F1;
    map[Key::F2] = ImGuiKey_F2;
    map[Key::F3] = ImGuiKey_F3;
    map[Key::F4] = ImGuiKey_F4;
    map[Key::F5] = ImGuiKey_F5;
    map[Key::F6] = ImGuiKey_F6;
    map[Key::F7] = ImGuiKey_F7;
    map[Key::F8] = ImGuiKey_F8;
    map[Key::F9] = ImGuiKey_F9;
    map[Key::F10] = ImGuiKey_F10;
    map[Key::F11] = ImGuiKey_F11;
    map[Key::F12] = ImGuiKey_F12;
    map[Key::NumPad0] = ImGuiKey_Keypad0;
    map[Key::NumPad1] = ImGuiKey_Keypad1;
    map[Key::NumPad2] = ImGuiKey_Keypad2;
    map[Key::NumPad3] = ImGuiKey_Keypad3;
    map[Key::NumPad4] = ImGuiKey_Keypad4;
    map[Key::NumPad5] = ImGuiKey_Keypad5;
    map[Key::NumPad6] = ImGuiKey_Keypad6;
    map[Key::NumPad7] = ImGuiKey_Keypad7;
    map[Key::NumPad8] = ImGuiKey_Keypad8;
    map[Key::NumPad9] = ImGuiKey_Keypad9;
    map[Key::Key0] = ImGuiKey_0;
    map[Key::Key1] = ImGuiKey_1;
    map[Key::Key2] = ImGuiKey_2;
    map[Key::Key3] = ImGuiKey_3;
    map[Key::Key4] = ImGuiKey_4;
    map[Key::Key5] = ImGuiKey_5;
    map[Key::Key6] = ImGuiKey_6;
    map[Key::Key7] = ImGuiKey_7;
    map[Key::Key8] = ImGuiKey_8;
    map[Key::Key9] = ImGuiKey_9;
    map[Key::KeyA] = ImGuiKey_A;
    map[Key::KeyB] = ImGuiKey_B;
    map[Key::KeyC] = ImGuiKey_C;
    map[Key::KeyD] = ImGuiKey_D;
    map[Key::KeyE] = ImGuiKey_E;
    map[Key::KeyF] = ImGuiKey_F;
    map[Key::KeyG] = ImGuiKey_G;
    map[Key::KeyH] = ImGuiKey_H;
    map[Key::KeyI] = ImGuiKey_I;
    map[Key::KeyJ] = ImGuiKey_J;
    map[Key::KeyK] = ImGuiKey_K;
    map[Key::KeyL] = ImGuiKey_L;
    map[Key::KeyM] = ImGuiKey_M;
    map[Key::KeyN] = ImGuiKey_N;
    map[Key::KeyO] = ImGuiKey_O;
    map[Key::KeyP] = ImGuiKey_P;
    map[Key::KeyQ] = ImGuiKey_Q;
    map[Key::KeyR] = ImGuiKey_R;
    map[Key::KeyS] = ImGuiKey_S;
    map[Key::KeyT] = ImGuiKey_T;
    map[Key::KeyU] = ImGuiKey_U;
    map[Key::KeyV] = ImGuiKey_V;
    map[Key::KeyW] = ImGuiKey_W;
    map[Key::KeyX] = ImGuiKey_X;
    map[Key::KeyY] = ImGuiKey_Y;
    map[Key::KeyZ] = ImGuiKey_Z;

//    io.ConfigFlags |= 0
//                      | ImGuiConfigFlags_NavEnableGamepad
//                      | ImGuiConfigFlags_NavEnableKeyboard
//            ;
//
//    m_keyMap[entry::Key::GamepadStart]     = ImGuiKey_GamepadStart;
//    m_keyMap[entry::Key::GamepadBack]      = ImGuiKey_GamepadBack;
//    m_keyMap[entry::Key::GamepadY]         = ImGuiKey_GamepadFaceUp;
//    m_keyMap[entry::Key::GamepadA]         = ImGuiKey_GamepadFaceDown;
//    m_keyMap[entry::Key::GamepadX]         = ImGuiKey_GamepadFaceLeft;
//    m_keyMap[entry::Key::GamepadB]         = ImGuiKey_GamepadFaceRight;
//    m_keyMap[entry::Key::GamepadUp]        = ImGuiKey_GamepadDpadUp;
//    m_keyMap[entry::Key::GamepadDown]      = ImGuiKey_GamepadDpadDown;
//    m_keyMap[entry::Key::GamepadLeft]      = ImGuiKey_GamepadDpadLeft;
//    m_keyMap[entry::Key::GamepadRight]     = ImGuiKey_GamepadDpadRight;
//    m_keyMap[entry::Key::GamepadShoulderL] = ImGuiKey_GamepadL1;
//    m_keyMap[entry::Key::GamepadShoulderR] = ImGuiKey_GamepadR1;
//    m_keyMap[entry::Key::GamepadThumbL]    = ImGuiKey_GamepadL3;
//    m_keyMap[entry::Key::GamepadThumbR]    = ImGuiKey_GamepadR3;

    return map;
}

struct FontRangeMerge {
    const void* data;
    size_t size;
    ImWchar ranges[3];
};

static FontRangeMerge s_fontRangeMerge[] =
        {
                {s_iconsKenneyTtf,      sizeof(s_iconsKenneyTtf),      {ICON_MIN_KI, ICON_MAX_KI, 0}},
                {s_iconsFontAwesomeTtf, sizeof(s_iconsFontAwesomeTtf), {ICON_MIN_FA, ICON_MAX_FA, 0}},
        };

static void* memAlloc(size_t _size, void* _userData);

static void memFree(void* _ptr, void* _userData);

struct OcornutImguiContext {
    void render(ImDrawData* _drawData) const {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width = (int) (_drawData->DisplaySize.x * _drawData->FramebufferScale.x);
        int fb_height = (int) (_drawData->DisplaySize.y * _drawData->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
            return;

        bgfx::setViewName(m_viewId, "ImGui");
        bgfx::setViewMode(m_viewId, bgfx::ViewMode::Sequential);

        const bgfx::Caps* caps = bgfx::getCaps();
        {
            float ortho[16];
            float x = _drawData->DisplayPos.x;
            float y = _drawData->DisplayPos.y;
            float width = _drawData->DisplaySize.x;
            float height = _drawData->DisplaySize.y;

            bx::mtxOrtho(ortho, x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
            bgfx::setViewTransform(m_viewId, NULL, ortho);
            bgfx::setViewRect(m_viewId, 0, 0, uint16_t(width), uint16_t(height));
        }

        const ImVec2 clipPos = _drawData->DisplayPos;       // (0,0) unless using multi-viewports
        const ImVec2 clipScale = _drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        for (int32_t ii = 0, num = _drawData->CmdListsCount; ii < num; ++ii) {
            bgfx::TransientVertexBuffer tvb;
            bgfx::TransientIndexBuffer tib;

            const ImDrawList* drawList = _drawData->CmdLists[ii];
            uint32_t numVertices = (uint32_t) drawList->VtxBuffer.size();
            uint32_t numIndices = (uint32_t) drawList->IdxBuffer.size();

            if (!checkAvailTransientBuffers(numVertices, m_layout, numIndices)) {
                // not enough space in transient buffer just quit drawing the rest...
                break;
            }

            bgfx::allocTransientVertexBuffer(&tvb, numVertices, m_layout);
            bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

            ImDrawVert* verts = (ImDrawVert*) tvb.data;
            bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

            ImDrawIdx* indices = (ImDrawIdx*) tib.data;
            bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

            bgfx::Encoder* encoder = bgfx::begin();

            for (const ImDrawCmd* cmd = drawList->CmdBuffer.begin(), * cmdEnd = drawList->CmdBuffer.end();
                 cmd != cmdEnd; ++cmd) {
                if (cmd->UserCallback) {
                    cmd->UserCallback(drawList, cmd);
                } else if (0 != cmd->ElemCount) {
                    uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;

                    bgfx::TextureHandle th = m_texture;
                    bgfx::ProgramHandle program = m_program;

                    if (NULL != cmd->TextureId) {
                        union {
                            ImTextureID ptr;
                            struct {
                                bgfx::TextureHandle handle;
                                uint8_t flags;
                                uint8_t mip;
                            } s;
                        } texture = {cmd->TextureId};
                        state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
                                 ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
                                 : BGFX_STATE_NONE;
                        th = texture.s.handle;
                        if (0 != texture.s.mip) {
                            const float lodEnabled[4] = {float(texture.s.mip), 1.0f, 0.0f, 0.0f};
                            bgfx::setUniform(u_imageLodEnabled, lodEnabled);
                            program = m_imageProgram;
                        }
                    } else {
                        state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
                    }

                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec4 clipRect;
                    clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
                    clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
                    clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
                    clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

                    if (clipRect.x < fb_width
                        && clipRect.y < fb_height
                        && clipRect.z >= 0.0f
                        && clipRect.w >= 0.0f) {
                        const uint16_t xx = uint16_t(bx::max(clipRect.x, 0.0f));
                        const uint16_t yy = uint16_t(bx::max(clipRect.y, 0.0f));
                        encoder->setScissor(xx, yy, uint16_t(bx::min(clipRect.z, 65535.0f) - xx),
                                            uint16_t(bx::min(clipRect.w, 65535.0f) - yy)
                        );

                        encoder->setState(state);
                        encoder->setTexture(0, s_tex, th);
                        encoder->setVertexBuffer(0, &tvb, cmd->VtxOffset, numVertices);
                        encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
                        encoder->submit(m_viewId, program);
                    }
                }
            }

            bgfx::end(encoder);
        }
    }

    void create(float _fontSize, bx::AllocatorI* _allocator) {
        m_allocator = _allocator;

        if (NULL == _allocator) {
            static bx::DefaultAllocator allocator;
            m_allocator = &allocator;
        }

        m_viewId = 255;
        m_lastScroll = 0;
        m_last = bx::getHPCounter();

        ImGui::SetAllocatorFunctions(memAlloc, memFree, NULL);

        m_imgui = ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();

        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DeltaTime = 1.0f / 60.0f;
        io.IniFilename = NULL;

        setupStyle(true);

        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard;

        bx::FileReader reader;
        m_program = bgfx::createProgram(
                nncc::engine::LoadShader(&reader, "vs_ocornut_imgui"),
                nncc::engine::LoadShader(&reader, "fs_ocornut_imgui"), true
        );

        u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
        m_imageProgram = bgfx::createProgram(
                nncc::engine::LoadShader(&reader, "vs_imgui_image"),
                nncc::engine::LoadShader(&reader, "fs_imgui_image"), true
        );

        m_layout
                .begin()
                .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
                .end();

        s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

        uint8_t* data;
        int32_t width;
        int32_t height;
        {
            ImFontConfig config;
            config.FontDataOwnedByAtlas = false;
            config.MergeMode = false;

            const ImWchar* ranges = io.Fonts->GetGlyphRangesCyrillic();
            m_font[ImGui::Font::Regular] = io.Fonts->AddFontFromMemoryTTF((void*) s_robotoRegularTtf,
                                                                          sizeof(s_robotoRegularTtf), _fontSize,
                                                                          &config, ranges);
            m_font[ImGui::Font::Mono] = io.Fonts->AddFontFromMemoryTTF((void*) s_robotoMonoRegularTtf,
                                                                       sizeof(s_robotoMonoRegularTtf), _fontSize - 3.0f,
                                                                       &config, ranges);

            config.MergeMode = true;
            config.DstFont = m_font[ImGui::Font::Regular];

            for (uint32_t ii = 0; ii < BX_COUNTOF(s_fontRangeMerge); ++ii) {
                const FontRangeMerge& frm = s_fontRangeMerge[ii];

                io.Fonts->AddFontFromMemoryTTF((void*) frm.data, (int) frm.size, _fontSize - 3.0f, &config, frm.ranges
                );
            }
        }

        io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

        m_texture = bgfx::createTexture2D(
                (uint16_t) width, (uint16_t) height, false, 1, bgfx::TextureFormat::BGRA8, 0,
                bgfx::copy(data, width * height * 4)
        );
    }

    void destroy() {
        ImGui::DestroyContext(m_imgui);

        bgfx::destroy(s_tex);
        bgfx::destroy(m_texture);

        bgfx::destroy(u_imageLodEnabled);
        bgfx::destroy(m_imageProgram);
        bgfx::destroy(m_program);

        m_allocator = NULL;
    }

    void setupStyle(bool _dark) {
        // Doug Binks' darl color scheme
        // https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9
        ImGuiStyle& style = ImGui::GetStyle();
        if (_dark) {
            ImGui::StyleColorsDark(&style);
        } else {
            ImGui::StyleColorsLight(&style);
        }

        style.FrameRounding = 4.0f;
        style.WindowBorderSize = 0.0f;
    }

    void beginFrame(
            int32_t _mx, int32_t _my, uint8_t _button, int32_t _scroll, int _width, int _height, int _inputChar,
            bgfx::ViewId _viewId
    ) {
        using namespace nncc::context;

        m_viewId = _viewId;

        ImGuiIO& io = ImGui::GetIO();
        if (_inputChar >= 0) {
            io.AddInputCharacter(_inputChar);
        }

        io.DisplaySize = ImVec2((float) _width, (float) _height);

        const int64_t now = bx::getHPCounter();
        const int64_t frameTime = now - m_last;
        m_last = now;
        const double freq = double(bx::getHPFrequency());
        io.DeltaTime = float(frameTime / freq);

        io.AddMousePosEvent((float) _mx, (float) _my);
        io.AddMouseButtonEvent(ImGuiMouseButton_Left, 0 != (_button & IMGUI_MBUT_LEFT));
        io.AddMouseButtonEvent(ImGuiMouseButton_Right, 0 != (_button & IMGUI_MBUT_RIGHT));
        io.AddMouseButtonEvent(ImGuiMouseButton_Middle, 0 != (_button & IMGUI_MBUT_MIDDLE));
        io.AddMouseWheelEvent(0.0f, (float) (_scroll - m_lastScroll));
        m_lastScroll = _scroll;

        const auto& key_state = nncc::context::Context::Get().key_state;

        auto modifiers = key_state.modifiers;
        io.AddKeyEvent(ImGuiKey_ModShift, 0 != (modifiers & (Modifier::LeftShift | Modifier::RightShift)));
        io.AddKeyEvent(ImGuiKey_ModCtrl, 0 != (modifiers & (Modifier::LeftCtrl | Modifier::RightCtrl)));
        io.AddKeyEvent(ImGuiKey_ModAlt, 0 != (modifiers & (Modifier::LeftAlt | Modifier::RightAlt)));
        io.AddKeyEvent(ImGuiKey_ModSuper, 0 != (modifiers & (Modifier::LeftMeta | Modifier::RightMeta)));

        for (const auto& key : key_state.pressed_keys) {
            if (!previous_pressed_keys.contains(key)) {
                io.AddKeyEvent(key_table[key], true);
            }
        }
        for (const auto& key : previous_pressed_keys) {
            if (!key_state.pressed_keys.contains(key)) {
                io.AddKeyEvent(key_table[key], false);
            }
        }
        previous_pressed_keys = key_state.pressed_keys;

        for (const auto& codepoint : nncc::context::Context::Get().input_characters) {
            io.AddInputCharacter(codepoint);
        }

        ImGui::NewFrame();
    }

    void endFrame() {
        ImGui::Render();
        render(ImGui::GetDrawData());
    }

    ImGuiContext* m_imgui;
    bx::AllocatorI* m_allocator;
    bgfx::VertexLayout m_layout;
    bgfx::ProgramHandle m_program;
    bgfx::ProgramHandle m_imageProgram;
    bgfx::TextureHandle m_texture;
    bgfx::UniformHandle s_tex;
    bgfx::UniformHandle u_imageLodEnabled;
    ImFont* m_font[ImGui::Font::Count];
    int64_t m_last;
    int32_t m_lastScroll;
    bgfx::ViewId m_viewId;

    // TODO: I need to pass key-up events properly, this needs a refactoring of key_state
    std::unordered_set<nncc::context::Key> previous_pressed_keys;
    std::unordered_map<nncc::context::Key, ImGuiKey_> key_table = MakeKeyTranslationTable();
};

static OcornutImguiContext context;

static void* memAlloc(size_t _size, void* _userData) {
    BX_UNUSED(_userData);
    return BX_ALLOC(context.m_allocator, _size);
}

static void memFree(void* _ptr, void* _userData) {
    BX_UNUSED(_userData);
    BX_FREE(context.m_allocator, _ptr);
}

void imguiCreate(float _fontSize, bx::AllocatorI* _allocator) {
    context.create(_fontSize, _allocator);
}

void imguiDestroy() {
    context.destroy();
}

void imguiBeginFrame(int32_t _mx, int32_t _my, uint8_t _button, int32_t _scroll, uint16_t _width, uint16_t _height,
                     int _inputChar, bgfx::ViewId _viewId) {
    context.beginFrame(_mx, _my, _button, _scroll, _width, _height, _inputChar, _viewId);
}

void imguiEndFrame() {
    context.endFrame();
}

namespace ImGui {
void PushFont(Font::Enum _font) {
    PushFont(context.m_font[_font]);
}

void PushEnabled(bool _enabled) {
    extern void PushItemFlag(int option, bool enabled);
    PushItemFlag(ImGuiItemFlags_Disabled, !_enabled);
    PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * (_enabled ? 1.0f : 0.5f));
}

void PopEnabled() {
    extern void PopItemFlag();
    PopItemFlag();
    PopStyleVar();
}

} // namespace ImGui

BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4505); // error C4505: '' : unreferenced local function has been removed

#define STBTT_malloc(_size, _userData) memAlloc(_size, _userData)
#define STBTT_free(_ptr, _userData) memFree(_ptr, _userData)

#define STB_RECT_PACK_IMPLEMENTATION

#include <3rdparty/stb/stb_rect_pack.h>

#define STB_TRUETYPE_IMPLEMENTATION

#include <3rdparty/stb/stb_truetype.h>
