#include "cameraPass.hpp"
#include "components.hpp"
#include "material.hpp"
#include "pass.hpp"
#include <cstdint>
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
                       const RenderResources& resources)
    : Pass(name, inputs, colorAttachments, depthInput, depthOutput, dependencies), materialHandle(materialHandle),
      resources(&resources) {}

void CameraPass::record(PassContext ctx) {

    // Depth write is determined by whether the pass has a meaningful depth output attachment
    bool depthWrite = false;
    const auto& depthOut = getDepthOutput();
    if (depthOut.isPresent() && depthOut.storeOp != vk::AttachmentStoreOp::eDontCare) {
        depthWrite = true;
    }

    std::vector<const Vulkan::DescriptorSetLayout*> layouts =
        materialHandle->getDescriptorSetLayout().has_value()
            ? std::vector<const Vulkan::DescriptorSetLayout*>{&*resources->sceneLayout,
                                                              &materialHandle->getDescriptorSetLayout().value()}
            : std::vector<const Vulkan::DescriptorSetLayout*>{&*resources->sceneLayout};

    auto& pipeline = ctx.pipelineManager->getOrCreatePipeline(
        materialHandle, layouts, getColorAttachmentFormats(), vk::SampleCountFlagBits::e1, depthWrite);
    ctx.cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);

    const std::array<vk::DescriptorSet, 1> rawSets = {**resources->sceneDescriptorSet};

    // Get active camera
    auto view = resources->entityRegistry->view<Scene::UBOArrayIndex, Scene::ActiveCamera>();
    const entt::entity activeCamera = view.front();
    auto& index = view.get<Scene::UBOArrayIndex>(activeCamera).index;

    ctx.cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               pipeline.getLayout(),
                               0,
                               rawSets,
                               {index * static_cast<uint32_t>(sizeof(CameraData))});

    ctx.cmd.draw(3, 1, 0, 0);
}
} // namespace Render