#pragma once

#include "instance.hpp"
#include "physicalDevice.hpp"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class Device {
  private:
    const vk::raii::PhysicalDevice* physicalDevice{};
    vk::raii::Device logicalDevice = nullptr;
    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::Queue presentQueue = nullptr;
    vk::raii::Queue computeQueue = nullptr;

    VmaAllocator memoryAllocator;

  public:
    Device(Vulkan::PhysicalDevice& physicalDevice, Vulkan::Instance& instance);
    [[nodiscard]] auto operator*() -> const vk::raii::Device& { return logicalDevice; }
    [[nodiscard]] auto operator->() -> vk::raii::Device* { return &logicalDevice; }
    [[nodiscard]] auto get() -> const vk::raii::Device&;
    auto getPhysicalDevice() -> const vk::raii::PhysicalDevice*;
    auto getGraphicsQueue() -> const vk::raii::Queue&;
    auto getPresentQueue() -> const vk::raii::Queue&;
    auto getAllocator() -> const VmaAllocator&;

    ~Device() {
        // Manually destroy VMA allocator before device destructs
        if (memoryAllocator != nullptr) {
            vmaDestroyAllocator(memoryAllocator);
            memoryAllocator = VK_NULL_HANDLE;
        }
    }

    Device(const Device&) = delete;
    auto operator=(const Device&) -> Device& = delete;
    Device(Device&&) = delete;
    auto operator=(Device&&) -> Device& = delete;
};
} // namespace Vulkan