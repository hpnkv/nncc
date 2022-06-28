#pragma once

#include <imgui/imgui.h>

#include <nncc/common/types.h>

struct InputTextCallback_UserData_Folly {
    nncc::string* Str;
    ImGuiInputTextCallback ChainCallback;
    void* ChainCallbackUserData;
};

static int InputTextCallbackFolly(ImGuiInputTextCallbackData* data) {
    InputTextCallback_UserData_Folly* user_data = (InputTextCallback_UserData_Folly*) data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
        nncc::string* str = user_data->Str;
        IM_ASSERT(data->Buf == str->c_str());
        str->resize(data->BufTextLen);
        data->Buf = (char*) str->c_str();
    } else if (user_data->ChainCallback) {
        // Forward to user callback, if any
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
    }
    return 0;
}

namespace ImGui {

IMGUI_API bool
InputText(const char* label, nncc::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL,
          void* user_data = NULL);

IMGUI_API bool InputTextMultiline(const char* label, nncc::string* str, const ImVec2& size = ImVec2(0, 0),
                                  ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL,
                                  void* user_data = NULL);

IMGUI_API bool InputTextWithHint(const char* label, const char* hint, nncc::string* str, ImGuiInputTextFlags flags = 0,
                                 ImGuiInputTextCallback callback = NULL, void* user_data = NULL);

}

