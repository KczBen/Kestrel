#include "material.hpp"
#include "descriptorSet.hpp"
#include "graphicsPipeline.hpp"
#include <cstdint>

Material::Material(Vulkan::Device& device,
                   Vulkan::Texture& albedo,
                   Vulkan::Texture& packedNormal,
                   Vulkan::GraphicsPipeline& pipeline,
                   Vulkan::DescriptorPool& descriptorPool,
                   Vulkan::DescriptorSetLayout& layout)
    : albedo(&albedo), packedNormal(&packedNormal), pipeline(&pipeline) {
    descriptorSets.reserve(2);

    for (uint32_t i = 0; i < 2; i++) {
        descriptorSets.emplace_back(device, descriptorPool, layout);

        const DescriptorWrite writes[] = {DescriptorWrite{.binding = 0,
                                                          .type = vk::DescriptorType::eCombinedImageSampler,
                                                          .imageInfo{albedo.getSampler(),
                                                                     albedo.getImage().getImageView(),
                                                                     vk::ImageLayout::eShaderReadOnlyOptimal}},
                                          DescriptorWrite{.binding = 1,
                                                          .type = vk::DescriptorType::eCombinedImageSampler,
                                                          .imageInfo{packedNormal.getSampler(),
                                                                     packedNormal.getImage().getImageView(),
                                                                     vk::ImageLayout::eShaderReadOnlyOptimal}}};

        descriptorSets[i].write(std::span(writes));
    }
}

auto Material::getDescriptorSet(uint32_t index) -> Vulkan::DescriptorSet& { return descriptorSets[index]; }
auto Material::getPipeline() -> Vulkan::GraphicsPipeline* { return pipeline; }