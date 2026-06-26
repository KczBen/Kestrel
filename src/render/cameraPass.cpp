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
                       Scene* scene,
                       Camera* camera)
    : Pass(pipelineManager, inputs, colorAttachments, depthInput, depthOutput), materialHandle(materialHandle),
      scene(scene), camera(camera) {}

void CameraPass::record(vk::CommandBuffer cmd) {

    // Depth write is determined by whether the pass has a meaningful depth output attachment
    bool depthWrite = false;
    const auto& depthOut = getDepthOutput();
    if (depthOut.isPresent() && depthOut.storeOp != vk::AttachmentStoreOp::eDontCare) {
        depthWrite = true;
    }

    auto& pipeline = getPipelineManager()->getOrCreatePipeline(
        materialHandle, *scene, getColorAttachmentFormats(), vk::SampleCountFlagBits::e1, depthWrite);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);

    const Vulkan::DescriptorSet& sceneWS = scene->getDescriptorSet();

    const std::array<vk::DescriptorSet, 1> rawSets = {*sceneWS};

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getLayout(), 0, rawSets, nullptr);

    cmd.draw(3, 1, 0, 0);
}
} // namespace Render