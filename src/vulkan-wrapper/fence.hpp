#pragma once

#include "device.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class Fence {
  private:
    const vk::raii::Device* device;
    vk::raii::Fence fence = nullptr;

  public:
    Fence(Vulkan::Device& device, vk::FenceCreateFlags flags);
    [[nodiscard]] auto operator*() const -> const vk::raii::Fence& { return fence; }
    [[nodiscard]] auto operator->() const -> const vk::raii::Fence* { return &fence; }
    auto get() -> const vk::raii::Fence&;
};
} // namespace Vulkan