#include "descriptorPool.hpp"
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

Vulkan::DescriptorPool::DescriptorPool(Vulkan::Device& device, uint32_t maxFramesInFlight) : device(&device.get()) {
    std::array poolSize{vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, maxFramesInFlight),
                        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, maxFramesInFlight)};

    const vk::DescriptorPoolCreateInfo poolInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, maxFramesInFlight, poolSize);

    pool = vk::raii::DescriptorPool(*this->device, poolInfo);
}

auto Vulkan::DescriptorPool::get() -> const vk::raii::DescriptorPool& { return pool; }