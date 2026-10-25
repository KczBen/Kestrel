#include "descriptorPool.hpp"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

Vulkan::DescriptorPool::DescriptorPool(Vulkan::Device& device) : device(&*device) {
    std::array poolSize{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
    };
    const vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 200, poolSize);

    pool = vk::raii::DescriptorPool(*this->device, poolInfo);
}

auto Vulkan::DescriptorPool::get() -> const vk::raii::DescriptorPool& { return pool; }