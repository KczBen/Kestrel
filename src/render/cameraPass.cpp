#include "cameraPass.hpp"
#include "material.hpp"
#include <vector>

namespace Render {

auto CameraPass::supportedMaterialTypes() const -> uint32_t {
    return (1u << static_cast<uint32_t>(Material::BlendMode::Opaque)) |
           (1u << static_cast<uint32_t>(Material::BlendMode::Alpha)) |
           (1u << static_cast<uint32_t>(Material::BlendMode::Premultiplied)) |
           (1u << static_cast<uint32_t>(Material::BlendMode::Additive));
}

CameraPass::CameraPass(Vulkan::PipelineManager& pipelineManager,
                       const std::vector<RenderTargetHandle>& targets,
                       const std::vector<RenderTargetHandle>& inputs,
                       MaterialHandle materialHandle,
                       Camera* camera)
    : Pass(pipelineManager, targets, inputs), materialHandle(materialHandle), camera(camera) {}

void CameraPass::record(vk::CommandBuffer cmd, uint32_t frameIndex) {
    camera->updateUniformBuffer(frameIndex);

    auto& pipeline = getPipelineManager()->getOrCreatePipeline(materialHandle, *camera, vk::Format::eB8G8R8A8Srgb);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());

    const Vulkan::DescriptorSet& cameraWS = camera->getDescriptorSet(frameIndex);

    const std::array<vk::DescriptorSet, 1> rawSets = {cameraWS.get()};

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getLayout(), 0, rawSets, nullptr);

    cmd.draw(3, 1, 0, 0);
}
} // namespace Render