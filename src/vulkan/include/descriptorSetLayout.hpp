#pragma once

#include "device.hpp"
#include "resourceHandles.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class DescriptorSetLayout {
  private:
    const vk::raii::Device* device;
    vk::raii::DescriptorSetLayout layout = nullptr;

  public:
    DescriptorSetLayout(Vulkan::Device& device, std::span<const vk::DescriptorSetLayoutBinding> bindings);
    [[nodiscard]] auto get() const -> const vk::raii::DescriptorSetLayout&;
};

using DescriptorSetLayoutHandle = Render::ResourceHandle<DescriptorSetLayout>;
} // namespace Vulkan