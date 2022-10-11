#include <nncc/context/context.h>
#include <nncc/gui/imgui/imgui_bgfx.h>

namespace nncc::python {

void Init() {
    auto& context = *nncc::context::Context::Get();
    ImGui::SetAllocatorFunctions(context.imgui_allocators.p_alloc_func, context.imgui_allocators.p_free_func);
    ImGui::SetCurrentContext(context.imgui_context);
}

}
