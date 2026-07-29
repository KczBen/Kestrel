#include "pipelineManager.hpp"
#include "descriptorPool.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "gpuDataObjects.hpp"
#include "graphicsPipeline.hpp"
#include "material.hpp"
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

        if (material.materialBufferHandle.isPresent()) {
            auto ubo = MaterialUBO{
                .baseColorFactor = {material.resources.baseColorFactor[0],
                                    material.resources.baseColorFactor[1],
                                    material.resources.baseColorFactor[2],
                                    material.resources.baseColorFactor[3]},
                .emissiveFactor = {material.resources.emissiveFactor[0],
                                   material.resources.emissiveFactor[1],
                                   material.resources.emissiveFactor[2],
                                   0.0f},
                .metallicFactor = material.resources.metallicFactor,
                .roughnessFactor = material.resources.roughnessFactor,
                .normalScale = material.resources.normalScale,
                .occlusionStrength = material.resources.occlusionStrength,
                .baseColorUVIndex = material.resources.baseColorUVIndex,
                .metallicRoughnessUVIndex = material.resources.metallicRoughnessUVIndex,
                .normalUVIndex = material.resources.normalUVIndex,
                .aoUVIndex = material.resources.aoUVIndex,
                .emissiveUVIndex = material.resources.emissiveUVIndex,
                .baseColorTransformMatrix = buildTextureTransformMatrix(material.resources.baseColorTransform),
                .metallicRoughnessTransformMatrix =
                    buildTextureTransformMatrix(material.resources.metallicRoughnessTransform),
                .normalTransformMatrix = buildTextureTransformMatrix(material.resources.normalTransform),
                .occlusionTransformMatrix = buildTextureTransformMatrix(material.resources.occlusionTransform),
                .emissiveTransformMatrix = buildTextureTransformMatrix(material.resources.emissiveTransform)};

            material.materialBufferHandle->update(&ubo, sizeof(MaterialUBO));

            writes.emplace_back(
                DescriptorWrite{.binding = bindingIndex++,
                                .type = vk::DescriptorType::eUniformBuffer,
                                .bufferInfo = {material.materialBufferHandle->get(), 0, sizeof(MaterialUBO)}});
        }

        if (material.resources.baseColorTexture.isPresent()) {
            writes.emplace_back(
                DescriptorWrite{.binding = bindingIndex++,
                                .type = vk::DescriptorType::eCombinedImageSampler,
                                .bufferInfo = {},
                                .imageInfo = {material.resources.baseColorTexture->getSampler(),
                                              material.resources.baseColorTexture->getImage().getImageView(),
                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.metallicRoughnessTexture.isPresent()) {
            writes.emplace_back(
                DescriptorWrite{.binding = bindingIndex++,
                                .type = vk::DescriptorType::eCombinedImageSampler,
                                .bufferInfo = {},
                                .imageInfo = {material.resources.metallicRoughnessTexture->getSampler(),
                                              material.resources.metallicRoughnessTexture->getImage().getImageView(),
                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.normalTexture.isPresent()) {
            writes.emplace_back(
                DescriptorWrite{.binding = bindingIndex++,
                                .type = vk::DescriptorType::eCombinedImageSampler,
                                .bufferInfo = {},
                                .imageInfo = {material.resources.normalTexture->getSampler(),
                                              material.resources.normalTexture->getImage().getImageView(),
                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.occlusionTexture.isPresent()) {
            writes.emplace_back(
                DescriptorWrite{.binding = bindingIndex++,
                                .type = vk::DescriptorType::eCombinedImageSampler,
                                .bufferInfo = {},
                                .imageInfo = {material.resources.occlusionTexture->getSampler(),
                                              material.resources.occlusionTexture->getImage().getImageView(),
                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }
        if (material.resources.emissiveTexture.isPresent()) {
            writes.emplace_back(
                DescriptorWrite{.binding = bindingIndex++,
                                .type = vk::DescriptorType::eCombinedImageSampler,
                                .bufferInfo = {},
                                .imageInfo = {material.resources.emissiveTexture->getSampler(),
                                              material.resources.emissiveTexture->getImage().getImageView(),
                                              vk::ImageLayout::eShaderReadOnlyOptimal}});
        }

        material.descriptorSet.value().write(std::span(writes));
    }
}

auto Vulkan::PipelineManager::getOrCreatePipeline(MaterialHandle materialHandle,
                                                  Scene::Scene& scene,
                                                  std::span<const vk::Format> formats,
                                                  vk::SampleCountFlagBits sampleCount,
                                                  bool depthWrite) -> Vulkan::GraphicsPipeline& {
    // Do we already know of a pipeline that may work?
    const PipelineKey key{materialHandle->pipelineConfiguration, formats, sampleCount, depthWrite};
    if (pipelineCache.contains(key)) {
        materialHandle->graphicsPipeline = &pipelineCache.find(key)->second;
        return *materialHandle->graphicsPipeline;
    }

    // always have the scene layout included (for now)
    std::vector<const Vulkan::DescriptorSetLayout*> layouts =
        materialHandle->descriptorSetLayout.has_value()
            ? std::vector<const Vulkan::DescriptorSetLayout*>{&scene.getDescriptorSetLayout(),
                                                              &materialHandle->descriptorSetLayout.value()}
            : std::vector<const Vulkan::DescriptorSetLayout*>{&scene.getDescriptorSetLayout()};

    vk::BlendOp blendMode = vk::BlendOp::eAdd;

    switch (materialHandle->pipelineConfiguration.blendMode) {

    case Material::BlendMode::Opaque:
        blendMode = vk::BlendOp::eAdd;
        break;
    case Material::BlendMode::Alpha:
        blendMode = vk::BlendOp::ePlusClampedAlphaEXT;
        break;
    case Material::BlendMode::Premultiplied:
        blendMode = vk::BlendOp::ePlusClampedAlphaEXT;
        break;
    case Material::BlendMode::Additive:
        blendMode = vk::BlendOp::eAdd;
        break;
    }

    auto newPipeline = Vulkan::GraphicsPipeline(*device,
                                                formats,
                                                layouts,
                                                materialHandle->pipelineConfiguration.pushConstants,
                                                materialHandle->pipelineConfiguration.vertexShader,
                                                materialHandle->pipelineConfiguration.fragmentShader,
                                                materialHandle->pipelineConfiguration.depthTest,
                                                depthWrite,
                                                materialHandle->pipelineConfiguration.cullMode,
                                                materialHandle->pipelineConfiguration.depthCompare,
                                                blendMode,
                                                materialHandle->pipelineConfiguration.colorWriteMask,
                                                sampleCount,
                                                materialHandle->pipelineConfiguration.vertexInputMode !=
                                                    Material::VertexInputMode::None);

    pipelineCache.insert({key, std::move(newPipeline)});
    materialHandle->graphicsPipeline = &pipelineCache.find(key)->second;

    return *materialHandle->graphicsPipeline;
}