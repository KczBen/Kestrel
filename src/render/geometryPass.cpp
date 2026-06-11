#include "geometryPass.hpp"
#include "material.hpp"
#include "object.hpp"
#include "pass.hpp"
#include <cstdint>
#include <unordered_map>

namespace Render {

auto GeometryPass::supportedMaterialTypes() const -> uint32_t {
    return 1u << static_cast<uint32_t>(Material::BlendMode::Opaque);
}

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

    struct DrawCall {
        Primitive primitive;
        glm::mat4 transform{};
    };

    // Build a list of draw calls
    // Key on the materials
    std::unordered_map<MaterialHandle, std::vector<DrawCall>> drawCalls;

    for (const auto& object : models) {
        auto node = object.model;

        auto buildDraws = [&](this auto& self, const Node& node, glm::mat4 parentTransform) -> void {
            const glm::mat4 worldTransform = parentTransform * node.getModelMatrix();

            for (const auto& primitive : node.primitives) {
                drawCalls[primitive.material].push_back({primitive, worldTransform});
            }

            for (const auto& child : node.children) {
                self(child, worldTransform);
            }
        };

        buildDraws(object.model, object.model.getModelMatrix());
    }

    for (const auto& [material, drawCalls] : drawCalls) {
        auto materialType = static_cast<uint32_t>(material->getBlendMode());
        if (!(supportedMaterialTypes() & (1u << materialType))) {
            continue;
        }

        auto& pipeline = pipelineManager->getOrCreatePipeline(material, *camera, vk::Format::eB8G8R8A8Srgb);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());

        const Vulkan::DescriptorSet& cameraWS = camera->getDescriptorSet(frameIndex);
        const Vulkan::DescriptorSet& materialWS = material->getDescriptorSet();

        const std::array<vk::DescriptorSet, 2> rawSets = {cameraWS.get(), materialWS.get()};

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getLayout(), 0, rawSets, nullptr);

        const vk::DeviceSize offsets[] = {0};

        for (const auto& drawCall : drawCalls) {

            cmd.bindVertexBuffers(0, drawCall.primitive.mesh.vertexBuffer->get(), offsets);
            cmd.bindIndexBuffer(drawCall.primitive.mesh.indexBuffer->get(), 0, vk::IndexType::eUint32);
            const PushConstants pc{drawCall.transform};
            cmd.pushConstants(
                pipeline.getLayout(), vk::ShaderStageFlagBits::eVertex, 0, vk::ArrayProxy<const PushConstants>(1, &pc));
            cmd.drawIndexed(drawCall.primitive.mesh.indexCount,
                            1,
                            drawCall.primitive.mesh.indexOffset,
                            static_cast<int32_t>(drawCall.primitive.mesh.vertexOffset),
                            0);
        }
    }
}
} // namespace Render