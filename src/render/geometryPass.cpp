#include "geometryPass.hpp"
#include "pass.hpp"
#include <cstdint>

namespace Render {
GeometryPass::GeometryPass(Vulkan::PipelineManager& pipelineManager, std::span<const Object> models, Camera* camera)
    : Pass(pipelineManager), models(models), camera(camera) {}

void GeometryPass::record(vk::CommandBuffer cmd, uint32_t frameIndex) {
    UniformBufferObject cameraUBO{};
    cameraUBO.view = camera->getViewMatrix();
    // Should get actual camera settings here
    cameraUBO.proj = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720, 0.1f, 100.0f);
    cameraUBO.proj[1][1] *= -1;
    cameraUBO.invView = glm::inverse(cameraUBO.view);
    cameraUBO.invProj = glm::inverse(cameraUBO.proj);

    camera->updateUniformBuffer(frameIndex, cameraUBO);

    for (const auto& model : models) {
        auto& pipeline = pipelineManager->getOrCreatePipeline(model.material, *camera, vk::Format::eB8G8R8A8Srgb);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());

        const Vulkan::DescriptorSet& cameraWS = camera->getDescriptorSet(frameIndex);
        const Vulkan::DescriptorSet& materialWS = model.material.get()->getDescriptorSet();

        const std::array<vk::DescriptorSet, 2> rawSets = {cameraWS.get(), materialWS.get()};

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getLayout(), 0, rawSets, nullptr);

        const vk::DeviceSize offsets[] = {0};
        cmd.bindVertexBuffers(0, model.mesh.vertexBuffer->get(), offsets);
        cmd.bindIndexBuffer(model.mesh.indexBuffer->get(), 0, vk::IndexType::eUint32);
        const PushConstants pc{model.getModelMatrix()};
        cmd.pushConstants(
            pipeline.getLayout(), vk::ShaderStageFlagBits::eVertex, 0, vk::ArrayProxy<const PushConstants>(1, &pc));
        cmd.drawIndexed(
            model.mesh.indexCount, 1, model.mesh.indexOffset, static_cast<int32_t>(model.mesh.vertexOffset), 0);
    }
}
} // namespace Render