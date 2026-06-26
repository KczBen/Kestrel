#include "geometryPass.hpp"
#include "material.hpp"
#include "object.hpp"
#include "pass.hpp"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Render {

auto GeometryPass::supportedMaterialTypes() const -> uint32_t {
    return 1u << static_cast<uint32_t>(Material::BlendMode::Opaque);
}

GeometryPass::GeometryPass(Vulkan::PipelineManager& pipelineManager,
                           const std::vector<RenderTargetAttachment>& inputs,
                           const std::vector<RenderTargetAttachment>& colorAttachments,
                           const RenderTargetAttachment& depthInput,
                           const RenderTargetAttachment& depthOutput,
                           std::optional<MaterialHandle> optMaterial,
                           std::span<const Object> models,
                           Camera* camera)
    : Pass(pipelineManager, inputs, colorAttachments, depthInput, depthOutput), models(models), camera(camera),
      optMaterial(optMaterial) {}

void GeometryPass::record(vk::CommandBuffer cmd, uint32_t frameIndex) {
    camera->updateUniformBuffer(frameIndex);

    struct DrawCall {
        Primitive primitive;
        MaterialHandle material;
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
                auto materialHandle = optMaterial.value_or(primitive.material);
                drawCalls[materialHandle].push_back({primitive, materialHandle, worldTransform});
            }

            for (const auto& child : node.children) {
                self(child, worldTransform);
            }
        };

        buildDraws(object.model, glm::mat4(1.0f));
    }

    const auto formats = getColorAttachmentFormats();

    // Depth write is determined by whether the pass has a meaningful depth output attachment
    bool depthWrite = false;
    const auto& depthOut = getDepthOutput();
    if (depthOut.isPresent() && depthOut.storeOp != vk::AttachmentStoreOp::eDontCare) {
        depthWrite = true;
    }

    for (const auto& [material, drawCalls] : drawCalls) {
        auto materialType = static_cast<uint32_t>(material->getBlendMode());
        if ((supportedMaterialTypes() & (1u << materialType)) == 0u) {
            continue;
        }

        auto& pipeline = getPipelineManager()->getOrCreatePipeline(
            material, *camera, formats, vk::SampleCountFlagBits::e1, depthWrite);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);

        const Vulkan::DescriptorSet& cameraWS = camera->getDescriptorSet(frameIndex);
        const Vulkan::DescriptorSet& materialWS = material->getDescriptorSet();

        const std::array<vk::DescriptorSet, 2> rawSets = {*cameraWS, *materialWS};

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getLayout(), 0, rawSets, nullptr);

        const vk::DeviceSize offsets[] = {0};

        for (const auto& drawCall : drawCalls) {

            cmd.bindVertexBuffers(0, drawCall.primitive.mesh.vertexBuffer->get(), offsets);
            cmd.bindIndexBuffer(drawCall.primitive.mesh.indexBuffer->get(), 0, vk::IndexType::eUint32);
            const glm::mat4 normalMatrix = glm::transpose(glm::inverse(drawCall.transform));
            const PushConstants pc{.model = drawCall.transform, .normalMatrix = normalMatrix};
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