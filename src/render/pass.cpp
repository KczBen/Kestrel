#include "deviceBuffer.hpp"
#include "gpuDataObjects.hpp"
#include "material.hpp"
#include "pipelineManager.hpp"
#include "vulkan/vulkan.hpp"

#include <cstdint>
#include <utility>

/*

Render::Pass::Pass(std::optional<Material> material,
                   std::optional<std::span<Vulkan::Image*>> inputImages,
                   std::optional<std::span<const Model* const>> inputModels,
                   std::optional<std::span<Camera*>> inputCameras,
                   std::optional<std::span<Vulkan::Image*>> outputImages,
                   Vulkan::PipelineManager& pipelineManager)
    : material(std::move(material)), inputImages(inputImages), inputModels(inputModels), inputCameras(inputCameras),
      outputImages(outputImages), pipelineManager(&pipelineManager) {

    if (inputModels && inputCameras) {
        type = RenderTypes::GEOMETRY;
    }

    else if (inputCameras && !inputModels) {
        type = RenderTypes::VIEW_ONLY;
    }

    else {
        type = RenderTypes::POST;
    }
}

void Render::Pass::record(const vk::CommandBuffer& cmd,
                          std::span<Material>& materialList,
                          std::optional<Vulkan::DeviceBuffer>& vertexBuffer,
                          std::optional<Vulkan::DeviceBuffer>& indexBuffer,
                          std::span<Vulkan::DescriptorSet>& cameraDescriptorSets,
                          uint32_t frameIndex) {
    switch (type) {
    case RenderTypes::GEOMETRY: {
        int32_t indexOffset = 0;
        int32_t vertexOffset = 0;
        // The renderer currently handles the UBOs. Not sure what the best course of action would be
        // Since we only have one camera as of now, pretend the list doesn't exist
        for (auto& material : materialList) {
            auto& pipeline = pipelineManager->getOrCreatePipeline(material, vk::Format::eB8G8R8A8Srgb);
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   pipeline.getLayout(),
                                   0,
                                   {*cameraDescriptorSets[frameIndex].get(), material.getDescriptorSet().get()},
                                   nullptr);

            for (const auto* model : inputModels.value()) {
                const vk::DeviceSize offsets[] = {0};
                cmd.bindVertexBuffers(0, vertexBuffer->get(), offsets);
                cmd.bindIndexBuffer(indexBuffer->get(), 0, vk::IndexType::eUint32);
                const PushConstants pc{model->getModelMatrix()};
                cmd.pushConstants(pipeline.getLayout(),
                                  vk::ShaderStageFlagBits::eVertex,
                                  0,
                                  vk::ArrayProxy<const PushConstants>(1, &pc));
                cmd.drawIndexed(model->indices.size(), 1, indexOffset, vertexOffset, 0);
                indexOffset += model->indices.size();
                vertexOffset += model->vertices.size();
            }
            indexOffset = 0;
            vertexOffset = 0;
        }

        return;
    }

    case RenderTypes::VIEW_ONLY: {
        auto& pipeline = pipelineManager->getOrCreatePipeline(material.value(), vk::Format::eB8G8R8A8Srgb);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               pipeline.getLayout(),
                               0,
                               {*cameraDescriptorSets[frameIndex].get()},
                               nullptr);

        cmd.draw(3, 1, 0, 0);

        return;
    }

    case RenderTypes::POST:
        break;
    }
}
*/