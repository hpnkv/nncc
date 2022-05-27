#pragma once

#include "surface.h"

#include <array>
#include <map>
#include <queue>
#include <vector>

#include <bgfx/bgfx.h>

namespace nncc::render {

const uint16_t kMaxTransientVertices = 0xFFFF;
const uint16_t kMaxTransientIndices = 0xFFFF;

template <class T>
struct Buffer {
    std::vector<T> content;
    size_t numel = 0;
};

struct BatchCommand {
    uint16_t vertex_start_index;
    uint16_t vertex_count;

    uint16_t index_start_index;
    uint16_t index_count;

    bgfx::ViewId view_id;
    uint64_t state;

    Material material;
    nncc::engine::Matrix4 transform;
};

struct BatchData {
    Buffer<PosNormUVVertex> transient_vertex_buffer;
    Buffer<uint16_t> transient_index_buffer;
    std::queue<BatchCommand> commands;
};

class BatchRenderer {
public:
    explicit BatchRenderer(const bgfx::RendererType::Enum& type = bgfx::RendererType::Noop) : type_(type) {}

    void Add(bgfx::ViewId view_id, const Mesh& mesh, const Material& material, const engine::Matrix4& transform, uint64_t state);

    void Init(bgfx::ProgramHandle shader_program);

    void Flush();

    void Prepare(bgfx::VertexLayout* vertex_layout);

private:
    bgfx::RendererType::Enum type_ = bgfx::RendererType::Noop;
    bgfx::VertexLayout* vertex_layout_ = nullptr;
    std::map<unsigned int, BatchData> batch_groups_ = {};
    bool initialised_ = false;

    bgfx::ProgramHandle shader_program_ = BGFX_INVALID_HANDLE;

    static bool CanBatch(BatchCommand* a, BatchCommand* b);

    BatchData* GetBatchData(uint16_t shader_idx, uint16_t texture_idx);

    void ResetBuffers ();
};

}