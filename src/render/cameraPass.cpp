#include "cameraPass.hpp"
#include "material.hpp"
#include "pass.hpp"
#include <vector>

namespace Render {

auto CameraPass::supportedMaterialTypes() const -> uint32_t {
    return (1U << static_cast<uint32_t>(Material::BlendMode::Opaque)) |
           (1U << static_cast<uint32_t>(Material::BlendMode::Alpha)) |
           (1U << static_cast<uint32_t>(Material::BlendMode::Premultiplied)) |
           (1U << static_cast<uint32_t>(Material::BlendMode::Additive));
}

CameraPass::CameraPass(const std::string& name,
                       const std::vector<RenderTargetAttachment>& inputs,
                       const std::vector<RenderTargetAttachment>& colorAttachments,
                       const RenderTargetAttachment& depthInput,
                       const RenderTargetAttachment& depthOutput,
                       const std::vector<std::string>& dependencies,
                       MaterialHandle materialHandle,
                       Scene::Scene* scene,
                       Camera* camera)
    : Pass(name, inputs, colorAttachments, depthInput, depthOutput, dependencies), materialHandle(materialHandle),
      scene(scene), camera(camera) {}

void CameraPass::record(PassContext ctx) {

    // Depth write is determined by whether the pass has a meaningful depth output attachment
    bool depthWrite = false;
    const auto& depthOut = getDepthOutput();
    if (depthOut.isPresent() && depthOut.storeOp != vk::AttachmentStoreOp::eDontCare) {
        depthWrite = true;
    }

    auto& pipeline = ctx.pipelineManager->getOrCreatePipeline(
        materialHandle, *scene, getColorAttachmentFormats(), vk::SampleCountFlagBits::e1, depthWrite);
    ctx.cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);

    const Vulkan::DescriptorSet& sceneWS = scene->getDescriptorSet();

    const std::array<vk::DescriptorSet, 1> rawSets = {*sceneWS};

    ctx.cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getLayout(), 0, rawSets, nullptr);

    ctx.cmd.draw(3, 1, 0, 0);
}
} // namespace Render