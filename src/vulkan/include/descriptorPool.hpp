#pragma once

#include "device.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class DescriptorPool {
  private:
    const vk::raii::Device* device;
    vk::raii::DescriptorPool pool = nullptr;

  public:
    DescriptorPool(Vulkan::Device& device);
    [[nodiscard]] auto operator*() const -> const vk::raii::DescriptorPool& { return pool; }
    [[nodiscard]] auto operator->() const -> const vk::raii::DescriptorPool* { return &pool; }
    auto get() -> const vk::raii::DescriptorPool&;
};
} // namespace Vulkan