#pragma once

#include "device.hpp"
#include "util/resourceHandles.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class DescriptorSetLayout {
  private:
    const vk::raii::Device* device;
    vk::raii::DescriptorSetLayout layout = nullptr;

  public:
    DescriptorSetLayout(Vulkan::Device& device, std::span<const vk::DescriptorSetLayoutBinding> bindings);
    [[nodiscard]] auto operator*() const -> const vk::raii::DescriptorSetLayout& { return layout; }
    [[nodiscard]] auto operator->() const -> const vk::raii::DescriptorSetLayout* { return &layout; }
    [[nodiscard]] auto get() const -> const vk::raii::DescriptorSetLayout&;
};

using DescriptorSetLayoutHandle = ResourceHandle<DescriptorSetLayout>;
} // namespace Vulkan