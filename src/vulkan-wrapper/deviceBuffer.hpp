#pragma once

#include "device.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"

namespace Vulkan {
class DeviceBuffer {
  private:
    VmaAllocator allocator{};
    vk::Buffer buffer;
    VmaAllocation allocation{};
    vk::DeviceSize bufferSize{};

  public:
    DeviceBuffer(Vulkan::Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage);

    ~DeviceBuffer() {
        if (buffer) {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    }

    DeviceBuffer(const DeviceBuffer&) = delete;
    auto operator=(const DeviceBuffer&) -> DeviceBuffer& = delete;

    DeviceBuffer(DeviceBuffer&& other) noexcept
        : allocator(other.allocator), buffer(other.buffer), allocation(other.allocation), bufferSize(other.bufferSize) {
        other.buffer = VK_NULL_HANDLE;
        other.allocation = nullptr;
    }

    auto operator=(DeviceBuffer&& other) noexcept -> DeviceBuffer& {
        if (this != &other) {
            if (buffer) {
                vmaDestroyBuffer(allocator, buffer, allocation);
            }
            allocator = other.allocator;
            buffer = other.buffer;
            allocation = other.allocation;
            bufferSize = other.bufferSize;
            other.buffer = VK_NULL_HANDLE;
            other.allocation = nullptr;
            other.bufferSize = 0;
        }
        return *this;
    }

    [[nodiscard]] auto operator*() const noexcept -> vk::Buffer { return buffer; }
    [[nodiscard]] auto get() const noexcept -> vk::Buffer;
    [[nodiscard]] auto size() const noexcept -> vk::DeviceSize;
};
} // namespace Vulkan