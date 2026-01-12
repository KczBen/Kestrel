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
        std::swap(buffer, other.buffer);
        std::swap(allocation, other.allocation);
        std::swap(bufferSize, other.bufferSize);
        std::swap(allocator, other.allocator);
        return *this;
    }

    [[nodiscard]] auto get() const noexcept -> vk::Buffer;
    [[nodiscard]] auto size() const noexcept -> vk::DeviceSize;
};
} // namespace Vulkan