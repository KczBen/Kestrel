#include "pipelineManager.hpp"
#include "descriptorPool.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "graphicsPipeline.hpp"
#include "vulkan/vulkan.hpp"
#include <utility>

Vulkan::PipelineManager::PipelineManager(Vulkan::Device& device, Vulkan::DescriptorPool& pool)
    : device(&device), pool(&pool) {}

void Vulkan::PipelineManager::registerMaterial(Material& material) {

    if (!material.pipelineConfiguration.descriptorSetBindings.empty()) {
        material.descriptorSetLayout.emplace(
            Vulkan::DescriptorSetLayout(*device, material.pipelineConfiguration.descriptorSetBindings));
        material.descriptorSet.emplace(Vulkan::DescriptorSet(*device, *pool, material.descriptorSetLayout.value()));

        std::vector<DescriptorWrite> writes;

        uint32_t bindingIndex = 0;

        if (material.resources.baseColor.isPresent()) {
            writes.emplace_back(DescriptorWrite{.binding = bindingIndex++,
                                                .type = vk::DescriptorType::eCombinedImageSampler,
                                                .bufferInfo = {},
                                                .imageInfo = {material.resources.baseColor->getSampler(),
                                                              material.resources.baseColor->getImage().getImageView(),
                                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.metallicRoughness.isPresent()) {
            writes.emplace_back(
                DescriptorWrite{.binding = bindingIndex++,
                                .type = vk::DescriptorType::eCombinedImageSampler,
                                .bufferInfo = {},
                                .imageInfo = {material.resources.metallicRoughness->getSampler(),
                                              material.resources.metallicRoughness->getImage().getImageView(),
                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.normal.isPresent()) {
            writes.emplace_back(DescriptorWrite{.binding = bindingIndex++,
                                                .type = vk::DescriptorType::eCombinedImageSampler,
                                                .bufferInfo = {},
                                                .imageInfo = {material.resources.normal->getSampler(),
                                                              material.resources.normal->getImage().getImageView(),
                                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.occlusion.isPresent()) {
            writes.emplace_back(DescriptorWrite{.binding = bindingIndex++,
                                                .type = vk::DescriptorType::eCombinedImageSampler,
                                                .bufferInfo = {},
                                                .imageInfo = {material.resources.occlusion->getSampler(),
                                                              material.resources.occlusion->getImage().getImageView(),
                                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.emissive.isPresent()) {
            writes.emplace_back(DescriptorWrite{.binding = bindingIndex++,
                                                .type = vk::DescriptorType::eCombinedImageSampler,
                                                .bufferInfo = {},
                                                .imageInfo = {material.resources.emissive->getSampler(),
                                                              material.resources.emissive->getImage().getImageView(),
                                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }

        material.descriptorSet.value().write(std::span(writes));
    }
}

auto Vulkan::PipelineManager::getOrCreatePipeline(MaterialHandle materialHandle,
                                                  Render::Camera& camera,
                                                  vk::Format format) -> Vulkan::GraphicsPipeline& {
    // If the last rendered format matches and we have a pipeline, we're all good
    if (materialHandle->graphicsPipeline != nullptr && materialHandle->lastRenderedFormat == format) {
        return *materialHandle->graphicsPipeline;
    }

    // Do we already know of a pipeline that may work?
    const PipelineKey key{materialHandle->pipelineConfiguration, format};
    if (pipelineCache.contains(key)) {
        materialHandle->graphicsPipeline = &pipelineCache.find(key)->second;
        return *materialHandle->graphicsPipeline;
    }

    // always have the camera layout included (for now)
    std::vector<const Vulkan::DescriptorSetLayout*> layouts =
        materialHandle->descriptorSetLayout.has_value()
            ? std::vector<const Vulkan::DescriptorSetLayout*>{&camera.getDescriptorSetLayout(),
                                                              &materialHandle->descriptorSetLayout.value()}
            : std::vector<const Vulkan::DescriptorSetLayout*>{&camera.getDescriptorSetLayout()};

    vk::BlendOp blendMode = vk::BlendOp::eAdd;

    switch (materialHandle->pipelineConfiguration.blendMode) {

    case Material::BlendMode::Opaque:
        blendMode = vk::BlendOp::eAdd;
    case Material::BlendMode::Alpha:
        blendMode = vk::BlendOp::ePlusClampedAlphaEXT;
    case Material::BlendMode::Premultiplied:
        blendMode = vk::BlendOp::ePlusClampedAlphaEXT;
    case Material::BlendMode::Additive:
        blendMode = vk::BlendOp::eAdd;
        break;
    }

    auto newPipeline = Vulkan::GraphicsPipeline(*device,
                                                format,
                                                layouts,
                                                materialHandle->pipelineConfiguration.pushConstants,
                                                materialHandle->pipelineConfiguration.vertexShader,
                                                materialHandle->pipelineConfiguration.fragmentShader,
                                                materialHandle->pipelineConfiguration.depthTest,
                                                materialHandle->pipelineConfiguration.depthWrite,
                                                materialHandle->pipelineConfiguration.cullMode,
                                                materialHandle->pipelineConfiguration.depthCompare,
                                                blendMode,
                                                materialHandle->pipelineConfiguration.colorWriteMask,
                                                materialHandle->pipelineConfiguration.vertexInputMode !=
                                                    Material::VertexInputMode::None);

    pipelineCache.insert({key, std::move(newPipeline)});
    materialHandle->graphicsPipeline = &pipelineCache.find(key)->second;

    return *materialHandle->graphicsPipeline;
}