#pragma once

#include "surface.h"

#include <array>
#include <map>
#include <tuple>
#include <queue>

#include <bgfx/bgfx.h>

#include <nncc/common/types.h>
#include <nncc/common/utils.h>

namespace nncc::rendering {

const uint16_t kMaxTransientVertices = 0xFFFF;
const uint16_t kMaxTransientIndices = 0xFFFF;

// https://stackoverflow.com/questions/52180053/efficient-and-simple-comparison-operators-for-structs
template<class T>
auto AsTuple(T&& object) -> decltype(object.AsTuple()) {
    return object.AsTuple();
}

template<class...Ts>
auto AsTuple(const std::tuple<Ts...>& tup) -> decltype(auto) {
    return tup;
}

template<class Lhs, class Rhs>
auto operator<(const Lhs& lhs, const Rhs& rhs)
-> decltype(AsTuple(lhs), void(), AsTuple(rhs), void(), bool()) {
    return AsTuple(lhs) < AsTuple(rhs);
}

template<class T>
struct Buffer {
    nncc::vector<T> content;
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
    nncc::math::Transform transform;
};

struct BatchData {
    Buffer<PosNormUVVertex> transient_vertex_buffer;
    Buffer<uint16_t> transient_index_buffer;
    std::queue<BatchCommand> commands;
};


class BatchRenderer {
public:
    explicit BatchRenderer(const bgfx::RendererType::Enum& type = bgfx::RendererType::Noop) : type_(type) {}

    void Add(bgfx::ViewId view_id, const Mesh& mesh, const Material& material, const math::Transform& transform,
             uint64_t state);

    void Init(bgfx::ProgramHandle shader_program);

    void Flush();

    void Prepare(bgfx::VertexLayout* vertex_layout);

    struct BatchId {
        uint16_t shader_idx;
        uint16_t texture_idx;

        auto AsTuple() const {
            return std::tie(shader_idx, texture_idx);
        }
    };

private:
    bgfx::RendererType::Enum type_ = bgfx::RendererType::Noop;
    bgfx::VertexLayout* vertex_layout_ = nullptr;
    std::map<BatchId, BatchData> batch_groups_ = {};
    bool initialised_ = false;

    bgfx::ProgramHandle shader_program_ = BGFX_INVALID_HANDLE;

    static bool CanBatch(BatchCommand* a, BatchCommand* b);

    BatchData* GetBatchData(const BatchId& batch_id);

    void ResetBuffers();
};

}