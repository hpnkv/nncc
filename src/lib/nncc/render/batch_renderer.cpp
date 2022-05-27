#include "batch_renderer.h"

#include <bgfx/bgfx.h>

namespace nncc::render {

void BatchRenderer::Add(bgfx::ViewId view_id, const Mesh& mesh, const Material& material, const engine::Matrix4& transform, uint64_t state) {
    // We can hard code the shader ID & the texture ID for now since we're not using it yet
    BatchData* batch;
    unsigned int batch_id = 1;
    if (!batch_groups_.count(batch_id)) {
        BatchData batch_data;
        batch_data.transient_vertex_buffer.content.resize(kMaxTransientVertices);
        batch_data.transient_index_buffer.content.resize(kMaxTransientIndices);
        batch_groups_[batch_id] = batch_data;
    }
    batch = &batch_groups_[batch_id];

    // Check if we have enough room in the buffer. If not, flush the buffer to the GPU before adding stuff
    if (mesh.vertices.size() + batch->transient_vertex_buffer.numel >= kMaxTransientVertices
        || mesh.vertices.size() + batch->transient_index_buffer.numel >= kMaxTransientIndices) {
        Flush();
    }

    // Make sure the batch renderer has been initialized
    Init(shader_program_);

    // Create new batch command
    BatchCommand command{};

    command.vertex_start_index = batch->transient_vertex_buffer.numel;
    command.vertex_count = mesh.vertices.size();

    command.index_start_index = batch->transient_index_buffer.numel;
    command.index_count = mesh.indices.size();

    command.view_id = view_id;
    command.state = state;

    command.material = material;
    command.transform = transform;

    // Copy vertex + indices to transient buffer
    for (auto i = 0; i < command.vertex_count; ++i) {
        ++batch->transient_vertex_buffer.numel;
        std::memcpy(&batch->transient_vertex_buffer.content[command.vertex_start_index + i], &mesh.vertices[i], sizeof(PosNormUVVertex));
    }

    int vertex_index_offset = 0;
    if (!batch->commands.empty() && CanBatch(&batch->commands.front(), &command)) {
        vertex_index_offset = command.vertex_start_index;
    }

    for (auto i = 0; i < command.index_count; ++i) {
        batch->transient_index_buffer.content[batch->transient_index_buffer.numel + i] = mesh.indices[i] + vertex_index_offset;
    }

    // Push batch command
    batch->commands.push(command);
}

void BatchRenderer::Flush() {
    uint16_t vertex_start  = 0;
    uint16_t vertex_count  = 0;

    uint16_t indices_start = 0;
    uint16_t indices_count = 0;

    uint64_t vertex_winding_direction = 0;
    if (type_ == bgfx::RendererType::Noop) {
        type_ = bgfx::getRendererType();
    }
//    if (type_ == bgfx::RendererType::Metal) {
//        vertex_winding_direction = BGFX_STATE_FRONT_CCW;
//    }

    bool is_batch = false;
    for (auto& [batch_id, batch] : batch_groups_) {
        if (batch.transient_vertex_buffer.numel == 0) {
            continue;
        }

        while (!batch.commands.empty()) {
            BatchCommand command = batch.commands.front();
            std::optional<BatchCommand> next_command;

            batch.commands.pop();

            if (!batch.commands.empty()) {
                next_command = batch.commands.front();
            }

            if (!is_batch) {
                is_batch = true;
                vertex_start = command.vertex_start_index;
                indices_start = command.index_start_index;
                indices_count = vertex_count = 0;
            }

            vertex_count += command.vertex_count;
            indices_count += command.index_count;

            // We can no longer batch the command. Send to GPU
            if (!next_command.has_value() || !CanBatch(&command, &*next_command)) {
                bgfx::TransientVertexBuffer tvb{};
                bgfx::allocTransientVertexBuffer(&tvb, vertex_count, PosNormUVVertex::layout);
                std::memcpy(tvb.data, batch.transient_vertex_buffer.content.data() + vertex_start, sizeof(PosNormUVVertex) * vertex_count);
                auto ibh = bgfx::createIndexBuffer( bgfx::makeRef(batch.transient_index_buffer.content.data() + indices_start, sizeof(uint16_t) * indices_count));

                bgfx::setVertexBuffer(0, &tvb);
                bgfx::setIndexBuffer(ibh);
                bgfx::setState(BGFX_STATE_DEFAULT | vertex_winding_direction);
                bgfx::setUniform(command.material.d_color_uniform, &command.material.diffuse_color);
                bgfx::setTexture(0, command.material.d_texture_uniform, command.material.diffuse_texture);
                bgfx::setTransform(*command.transform);

                bgfx::submit(command.view_id, command.material.shader);
                bgfx::destroy(ibh);
                is_batch = false;
            }
        }
    }

    // Done!
    ResetBuffers();
    initialised_ = false;
}

void BatchRenderer::Prepare(bgfx::VertexLayout* vertex_layout) {
    if (vertex_layout_ != nullptr)
        return;
    vertex_layout_ = vertex_layout;
    ResetBuffers();
}

bool BatchRenderer::CanBatch(nncc::render::BatchCommand* a, nncc::render::BatchCommand* b) {
    return a->state == b->state;
}

BatchData* BatchRenderer::GetBatchData(uint16_t shader_idx, uint16_t texture_idx) {
    unsigned int batch_id = shader_idx * texture_idx;
    if (batch_groups_.count(batch_id)) {
        return &batch_groups_[batch_id];
    }

    auto [map_it, inserted] = batch_groups_.emplace(batch_id, BatchData());
    return &map_it->second;
}

void BatchRenderer::ResetBuffers() {
    for (auto& [idx, data] : batch_groups_) {
        data.transient_vertex_buffer.numel = 0;
        data.transient_index_buffer.numel = 0;
    }
}

void BatchRenderer::Init(bgfx::ProgramHandle shader_program) {
    if (initialised_) {
        return;
    }

    shader_program_ = shader_program;
    if (shader_program_.idx != bgfx::kInvalidHandle) {
        initialised_ = true;
    }
}

}
