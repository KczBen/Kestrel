#include "geometryPass.hpp"
#include "../scene/include/components.hpp"
#include "material.hpp"
#include "pass.hpp"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Render {

auto GeometryPass::supportedMaterialTypes() const -> uint32_t {
    return 1u << static_cast<uint32_t>(Material::BlendMode::Opaque);
}

GeometryPass::GeometryPass(const std::string& name,
                           const std::vector<RenderTargetAttachment>& inputs,
                           const std::vector<RenderTargetAttachment>& colorAttachments,
                           const RenderTargetAttachment& depthInput,
                           const RenderTargetAttachment& depthOutput,
                           const std::vector<std::string>& dependencies,
                           std::optional<MaterialHandle> optMaterial,
                           Scene* scene,
                           Camera* camera)
    : Pass(name, inputs, colorAttachments, depthInput, depthOutput, dependencies), scene(scene), camera(camera),
      optMaterial(optMaterial) {}

void GeometryPass::record(PassContext ctx) {

    struct DrawCall {
        ::Scene::Primitive primitive;
        MaterialHandle material;
        glm::mat4 transform{};
    };

    auto& registry = scene->getRegistry();

    // Build a list of draw calls
    // Key on the materials
    std::unordered_map<MaterialHandle, std::vector<DrawCall>> drawCalls;

    auto view = registry.view<::Scene::Object, ::Scene::Children>();

    auto buildDraws = [&](this auto& self, entt::entity entity, glm::mat4 parentTransform) -> void {
        auto& localTransform = registry.get<::Scene::LocalTransform>(entity);
        glm::mat4 localMatrix;
        localMatrix = glm::translate(glm::mat4(1.0f), localTransform.position);
        localMatrix *= glm::mat4_cast(localTransform.rotation);
        localMatrix = glm::scale(localMatrix, localTransform.scale);

        const glm::mat4 worldTransform = parentTransform * localMatrix;

        if (registry.any_of<::Scene::Primitive>(entity)) {
            auto& primitive = registry.get<::Scene::Primitive>(entity);
            auto materialHandle = optMaterial.value_or(primitive.material);
            drawCalls[materialHandle].push_back({primitive, materialHandle, worldTransform});
        }

        auto& children = registry.get<::Scene::Children>(entity);
        for (auto child : *children) {
            if (registry.any_of<::Scene::Node>(child)) {
                self(child, worldTransform);
            } else if (registry.any_of<::Scene::Primitive>(child)) {
                auto& primitive = registry.get<::Scene::Primitive>(child);
                auto materialHandle = optMaterial.value_or(primitive.material);
                drawCalls[materialHandle].push_back({primitive, materialHandle, worldTransform});
            }
        }
    };

    for (auto entity : view) {
        buildDraws(entity, glm::mat4(1.0f));
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

        auto& pipeline = ctx.pipelineManager->getOrCreatePipeline(
            material, *scene, formats, vk::SampleCountFlagBits::e1, depthWrite);
        ctx.cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);

        const Vulkan::DescriptorSet& sceneWS = scene->getDescriptorSet();
        const Vulkan::DescriptorSet& materialWS = material->getDescriptorSet();

        const std::array<vk::DescriptorSet, 2> rawSets = {*sceneWS, *materialWS};

        ctx.cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getLayout(), 0, rawSets, nullptr);

        const vk::DeviceSize offsets[] = {0};

        for (const auto& drawCall : drawCalls) {

            ctx.cmd.bindVertexBuffers(0, drawCall.primitive.mesh.vertexBuffer->get(), offsets);
            ctx.cmd.bindIndexBuffer(drawCall.primitive.mesh.indexBuffer->get(), 0, vk::IndexType::eUint32);
            const glm::mat4 normalMatrix = glm::transpose(glm::inverse(drawCall.transform));
            const PushConstants pc{.model = drawCall.transform, .normalMatrix = normalMatrix};
            ctx.cmd.pushConstants(
                pipeline.getLayout(), vk::ShaderStageFlagBits::eVertex, 0, vk::ArrayProxy<const PushConstants>(1, &pc));
            ctx.cmd.drawIndexed(drawCall.primitive.mesh.indexCount,
                                1,
                                drawCall.primitive.mesh.indexOffset,
                                static_cast<int32_t>(drawCall.primitive.mesh.vertexOffset),
                                0);
        }
    }
}
} // namespace Render