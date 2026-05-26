#include "material.hpp"
#include "descriptorSet.hpp"
#include "vulkan/vulkan.hpp"
#include <unistd.h>

#include <utility>
#include <vector>

Material::Material(ShaderDef vertexShader,
                   ShaderDef fragmentShader,
                   const MaterialResources& materialResources,
                   std::vector<vk::PushConstantRange> pushConstants,
                   VertexInputMode vertexInputMode,
                   vk::CullModeFlagBits cullMode,
                   bool depthTest,
                   bool depthWrite,
                   vk::CompareOp depthCompare,
                   BlendMode colorBlendMode,
                   vk::ColorComponentFlags colorWriteMask)
    : resources(materialResources) {

    // Retain full, explicit control over pipeline state for flexibility
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

auto Material::getDescriptorSet() -> Vulkan::DescriptorSet& { return descriptorSet.value(); }