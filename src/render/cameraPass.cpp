#include "cameraPass.hpp"

namespace Render {
CameraPass::CameraPass(Vulkan::PipelineManager& pipelineManager,
                       std::vector<Material>& materialList,
                       MaterialHandle materialHandle,
                       Camera* camera)
    : Pass(pipelineManager), materialList(&materialList), materialHandle(materialHandle), camera(camera) {}

void CameraPass::record(vk::CommandBuffer cmd, uint32_t frameIndex) {
    UniformBufferObject cameraUBO{};
    cameraUBO.view = camera->getViewMatrix();
    // Should get actual camera settings here
    cameraUBO.proj = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 100.0f);
    cameraUBO.proj[1][1] *= -1;
    cameraUBO.invView = glm::inverse(cameraUBO.view);
    cameraUBO.invProj = glm::inverse(cameraUBO.proj);

    camera->updateUniformBuffer(frameIndex, cameraUBO);

    auto& pipeline = pipelineManager->getOrCreatePipeline(materialHandle, *camera, vk::Format::eB8G8R8A8Srgb);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());

    const Vulkan::DescriptorSet& cameraWS = camera->getDescriptorSet(frameIndex);

    const std::array<vk::DescriptorSet, 1> rawSets = {cameraWS.get()};

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getLayout(), 0, rawSets, nullptr);

    cmd.draw(3, 1, 0, 0);
}
} // namespace Render