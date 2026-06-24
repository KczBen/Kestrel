#include "cameraPass.hpp"
#include "material.hpp"
#include <vector>

namespace Render {

auto CameraPass::supportedMaterialTypes() const -> uint32_t {
    return (1U << static_cast<uint32_t>(Material::BlendMode::Opaque)) |
           (1U << static_cast<uint32_t>(Material::BlendMode::Alpha)) |
           (1U << static_cast<uint32_t>(Material::BlendMode::Premultiplied)) |
           (1U << static_cast<uint32_t>(Material::BlendMode::Additive));
}

CameraPass::CameraPass(Vulkan::PipelineManager& pipelineManager,
                       const std::vector<RenderTargetAttachment>& inputs,
                       const std::vector<RenderTargetAttachment>& colorAttachments,
                       const RenderTargetAttachment& depthInput,
                       const RenderTargetAttachment& depthOutput,
                       MaterialHandle materialHandle,
                       Camera* camera)
    : Pass(pipelineManager, inputs, colorAttachments, depthInput, depthOutput), materialHandle(materialHandle),
      camera(camera) {}

void CameraPass::record(vk::CommandBuffer cmd, uint32_t frameIndex) {
    camera->updateUniformBuffer(frameIndex);

    auto& pipeline = getPipelineManager()->getOrCreatePipeline(materialHandle, *camera, vk::Format::eB8G8R8A8Srgb);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);

    const Vulkan::DescriptorSet& cameraWS = camera->getDescriptorSet(frameIndex);

    const std::array<vk::DescriptorSet, 1> rawSets = {*cameraWS};

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getLayout(), 0, rawSets, nullptr);

    cmd.draw(3, 1, 0, 0);
}
} // namespace Render