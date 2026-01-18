#include "descriptorSetLayout.hpp"
#include <vulkan/vulkan_raii.hpp>

Vulkan::DescriptorSetLayout::DescriptorSetLayout(Vulkan::Device& device,
                                                 std::span<const vk::DescriptorSetLayoutBinding> bindings)
    : device(&device.get()) {
    const vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings.size(), bindings.data());

    layout = vk::raii::DescriptorSetLayout(*this->device, layoutInfo);
}

auto Vulkan::DescriptorSetLayout::get() const -> const vk::raii::DescriptorSetLayout& { return layout; }