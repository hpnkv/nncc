#pragma once

#ifdef NNCC_SHARED_CONTEXT_IMPLEMENTATION

namespace nncc::context {

void InitShared() {
    auto& context = *nncc::context::Context::Get();
    ImGui::SetAllocatorFunctions(context.imgui_allocators.p_alloc_func, context.imgui_allocators.p_free_func);
    ImGui::SetCurrentContext(context.imgui_context);
}

}

#endif
