#include "pipelineManager.hpp"
#include "descriptorPool.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "graphicsPipeline.hpp"
#include "vulkan/vulkan.hpp"
#include <utility>

Vulkan::PipelineManager::PipelineManager(Vulkan::Device& device,
                                         Vulkan::DescriptorSetLayout& cameraLayout,
                                         Vulkan::DescriptorPool& pool)
    : device(&device), cameraLayout(&cameraLayout), pool(&pool) {}

void Vulkan::PipelineManager::registerMaterial(Material& material) {
    // Do we need descriptor sets
    if (!material.pipelineConfiguration.descriptorSetBindings.empty()) {
        material.descriptorSetLayout.emplace(
            Vulkan::DescriptorSetLayout(*device, material.pipelineConfiguration.descriptorSetBindings));
        material.descriptorSet.emplace(Vulkan::DescriptorSet(*device, *pool, material.descriptorSetLayout.value()));

        std::vector<DescriptorWrite> writes;

        uint32_t bindingIndex = 0;

        if (material.resources.baseColor != nullptr) {
            writes.emplace_back(DescriptorWrite{.binding = bindingIndex++,
                                                .type = vk::DescriptorType::eCombinedImageSampler,
                                                .bufferInfo = {},
                                                .imageInfo = {material.resources.baseColor->getSampler(),
                                                              material.resources.baseColor->getImage().getImageView(),
                                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.metallicRoughness != nullptr) {
            writes.emplace_back(
                DescriptorWrite{.binding = bindingIndex++,
                                .type = vk::DescriptorType::eCombinedImageSampler,
                                .bufferInfo = {},
                                .imageInfo = {material.resources.metallicRoughness->getSampler(),
                                              material.resources.metallicRoughness->getImage().getImageView(),
                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.normal != nullptr) {
            writes.emplace_back(DescriptorWrite{.binding = bindingIndex++,
                                                .type = vk::DescriptorType::eCombinedImageSampler,
                                                .bufferInfo = {},
                                                .imageInfo = {material.resources.normal->getSampler(),
                                                              material.resources.normal->getImage().getImageView(),
                                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.occlusion != nullptr) {
            writes.emplace_back(DescriptorWrite{.binding = bindingIndex++,
                                                .type = vk::DescriptorType::eCombinedImageSampler,
                                                .bufferInfo = {},
                                                .imageInfo = {material.resources.occlusion->getSampler(),
                                                              material.resources.occlusion->getImage().getImageView(),
                                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.emissive != nullptr) {
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

auto Vulkan::PipelineManager::getOrCreatePipeline(Material& material, vk::Format format) -> Vulkan::GraphicsPipeline& {
    // If the last rendered format matches and we have a pipeline, we're all good
    if (material.graphicsPipeline != nullptr && material.lastRenderedFormat == format) {
        return *material.graphicsPipeline;
    }

    // Do we already know of a pipeline that may work?
    const PipelineKey key{material.pipelineConfiguration, format};
    if (pipelineCache.contains(key)) {
        material.graphicsPipeline = &pipelineCache.find(key)->second;
        return *material.graphicsPipeline;
    }

    // always have the camera layout included (for now)
    std::vector<const Vulkan::DescriptorSetLayout*> layouts =
        material.descriptorSetLayout.has_value()
            ? std::vector<const Vulkan::DescriptorSetLayout*>{cameraLayout, &material.descriptorSetLayout.value()}
            : std::vector<const Vulkan::DescriptorSetLayout*>{cameraLayout};

    vk::BlendOp blendMode = vk::BlendOp::eAdd;

    switch (material.pipelineConfiguration.blendMode) {

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

    auto newPipeline =
        Vulkan::GraphicsPipeline(*device,
                                 format,
                                 layouts,
                                 material.pipelineConfiguration.pushConstants,
                                 material.pipelineConfiguration.vertexShader,
                                 material.pipelineConfiguration.fragmentShader,
                                 material.pipelineConfiguration.depthTest,
                                 material.pipelineConfiguration.depthWrite,
                                 material.pipelineConfiguration.cullMode,
                                 material.pipelineConfiguration.depthCompare,
                                 blendMode,
                                 material.pipelineConfiguration.colorWriteMask,
                                 material.pipelineConfiguration.vertexInputMode != Material::VertexInputMode::None);

    pipelineCache.insert({key, std::move(newPipeline)});
    material.graphicsPipeline = &pipelineCache.find(key)->second;

    return *material.graphicsPipeline;
}