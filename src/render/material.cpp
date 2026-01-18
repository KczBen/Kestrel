#include "material.hpp"
#include "descriptorSet.hpp"
#include "vulkan/vulkan.hpp"
#include <unistd.h>

#include <utility>
#include <vector>

Material::Material(ShaderDef vertexShader,
                   ShaderDef fragmentShader,
                   Vulkan::Texture* baseColor,
                   Vulkan::Texture* metallicRoughness,
                   Vulkan::Texture* normal,
                   Vulkan::Texture* occlusion,
                   Vulkan::Texture* emissive,
                   std::vector<vk::PushConstantRange> pushConstants,
                   VertexInputMode vertexInputMode,
                   vk::CullModeFlagBits cullMode,
                   bool depthTest,
                   bool depthWrite,
                   vk::CompareOp depthCompare,
                   BlendMode colorBlendMode,
                   vk::ColorComponentFlags colorWriteMask) {

    pipelineConfiguration = MaterialConfiguration{.vertexShader = std::move(vertexShader),
                                                  .fragmentShader = std::move(fragmentShader),
                                                  .vertexInputMode = vertexInputMode,
                                                  .topology = vk::PrimitiveTopology::eTriangleList,
                                                  .cullMode = cullMode,
                                                  .depthTest = depthTest,
                                                  .depthWrite = depthWrite,
                                                  .depthCompare = depthCompare,
                                                  .blendMode = colorBlendMode,
                                                  .colorWriteMask = colorWriteMask,
                                                  .pushConstants = std::move(pushConstants)};

    resources = MaterialResources{.baseColor = baseColor,
                                  .metallicRoughness = metallicRoughness,
                                  .normal = normal,
                                  .occlusion = occlusion,
                                  .emissive = emissive};

    int bindingIndex = 0;

    if (resources.baseColor != nullptr) {
        pipelineConfiguration.descriptorSetBindings.emplace_back(
            bindingIndex++, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    }
    if (resources.metallicRoughness != nullptr) {
        pipelineConfiguration.descriptorSetBindings.emplace_back(
            bindingIndex++, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    }
    if (resources.normal != nullptr) {
        pipelineConfiguration.descriptorSetBindings.emplace_back(
            bindingIndex++, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    }
    if (resources.occlusion != nullptr) {
        pipelineConfiguration.descriptorSetBindings.emplace_back(
            bindingIndex++, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    }
    if (resources.emissive != nullptr) {
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
           this->descriptorSetBindings == other.descriptorSetBindings;
}

auto Material::getDescriptorSet() -> Vulkan::DescriptorSet& { return descriptorSet.value(); }