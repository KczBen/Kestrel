#pragma once

#include "device.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class Semaphore {
  private:
    const vk::raii::Device* device;
    vk::raii::Semaphore semaphore = nullptr;

  public:
    Semaphore(Vulkan::Device& device);
    [[nodiscard]] auto operator*() const -> const vk::raii::Semaphore& { return semaphore; }
    [[nodiscard]] auto operator->() const -> const vk::raii::Semaphore* { return &semaphore; }
    auto get() -> const vk::raii::Semaphore&;
};
} // namespace Vulkan