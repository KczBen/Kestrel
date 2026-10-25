#include "fence.hpp"
#include <vulkan/vulkan_raii.hpp>

Vulkan::Fence::Fence(Vulkan::Device& device, vk::FenceCreateFlags flags) : device(&*device) {
    const vk::FenceCreateInfo fenceInfo(flags);

    fence = vk::raii::Fence(*this->device, fenceInfo);
}

auto Vulkan::Fence::get() -> const vk::raii::Fence& { return fence; }