#include "material.hpp"
#include "vulkan-wrapper/descriptorSet.hpp"
#include "vulkan-wrapper/descriptorSetLayout.hpp"

#include <optional>
#include <unistd.h>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

Material::Material(std::string name, MaterialConfiguration config, MaterialResources resources)
    : name(std::move(name)), resources(std::move(resources)), pipelineConfiguration(std::move(config)) {

    uint32_t bindingIndex = 0;

    pipelineConfiguration.descriptorSetBindings.emplace_back(
        bindingIndex++, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment);

    if (resources.baseColorTexture.isPresent()) {
        pipelineConfiguration.descriptorSetBindings.emplace_back(
            bindingIndex++, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    }
    if (resources.metallicRoughnessTexture.isPresent()) {
        pipelineConfiguration.descriptorSetBindings.emplace_back(
            bindingIndex++, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    }
    if (resources.normalTexture.isPresent()) {
        pipelineConfiguration.descriptorSetBindings.emplace_back(
            bindingIndex++, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    }
    if (resources.occlusionTexture.isPresent()) {
        pipelineConfiguration.descriptorSetBindings.emplace_back(
            bindingIndex++, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    }
    if (resources.emissiveTexture.isPresent()) {
        pipelineConfiguration.descriptorSetBindings.emplace_back(
            bindingIndex++, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    }
}

auto Material::MaterialConfiguration::operator==(const MaterialConfiguration& other) const -> bool {
    return this->vertexShader.filePath == other.vertexShader.filePath &&
           this->vertexShader.stage == other.vertexShader.stage &&
           this->fragmentShader.filePath == other.fragmentShader.filePath &&
           this->fragmentShader.stage == other.fragmentShader.stage && this->vertexInputMode == other.vertexInputMode &&
           this->blendMode == other.blendMode && this->colorWriteMask == other.colorWriteMask &&
           this->cullMode == other.cullMode && this->topology == other.topology && this->depthTest == other.depthTest &&
           this->depthWrite == other.depthWrite && this->depthCompare == other.depthCompare &&
           this->descriptorSetBindings == other.descriptorSetBindings && this->pushConstants == other.pushConstants;
}

auto Material::MaterialResources::operator==(const MaterialResources& other) const -> bool {
    return baseColorTexture == other.baseColorTexture && metallicRoughnessTexture == other.metallicRoughnessTexture &&
           normalTexture == other.normalTexture && occlusionTexture == other.occlusionTexture &&
           emissiveTexture == other.emissiveTexture && baseColorUVIndex == other.baseColorUVIndex &&
           metallicRoughnessUVIndex == other.metallicRoughnessUVIndex && normalUVIndex == other.normalUVIndex &&
           aoUVIndex == other.aoUVIndex && emissiveUVIndex == other.emissiveUVIndex &&
           baseColorTransform == other.baseColorTransform &&
           metallicRoughnessTransform == other.metallicRoughnessTransform && normalTransform == other.normalTransform &&
           occlusionTransform == other.occlusionTransform && emissiveTransform == other.emissiveTransform &&
           baseColorFactor == other.baseColorFactor && emissiveFactor == other.emissiveFactor &&
           metallicFactor == other.metallicFactor && roughnessFactor == other.roughnessFactor &&
           normalScale == other.normalScale && occlusionStrength == other.occlusionStrength;
}

auto Material::getDescriptorSet() -> Vulkan::DescriptorSet& { return descriptorSet.value(); }
auto Material::getDescriptorSetLayout() -> std::optional<Vulkan::DescriptorSetLayout>& { return descriptorSetLayout; }