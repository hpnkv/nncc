#pragma once

#include <numeric>

#include <bgfx/bgfx.h>
#include <torch/torch.h>
#include <libshm/libshm.h>

#include <nncc/common/types.h>
#include <nncc/context/context.h>
#include <nncc/render/renderer.h>
#include <nncc/render/primitives.h>

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

        tensor_ = torch::from_blob(data_ptr_.get(), {dims[0], dims[1], dims[2]});

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
        renderer->Add(render::kPlaneMesh, material_, transform_);
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

