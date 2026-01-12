#pragma once

#include "device.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"

namespace Vulkan {
class HostBuffer {
  private:
    VmaAllocator allocator;
    vk::Buffer buffer;
    VmaAllocation allocation{};
    void* mapped{};
    vk::DeviceSize bufferSize{};

  public:
    HostBuffer(Vulkan::Device& device, vk::DeviceSize size);

    ~HostBuffer() {
        if (buffer) {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    }

    HostBuffer(const HostBuffer&) = delete;
    auto operator=(const HostBuffer&) -> HostBuffer& = delete;

    HostBuffer(HostBuffer&& other) noexcept
        : allocator(other.allocator), buffer(other.buffer), allocation(other.allocation), bufferSize(other.bufferSize) {
        other.buffer = VK_NULL_HANDLE;
        other.allocation = nullptr;
    }

    auto operator=(HostBuffer&& other) noexcept -> HostBuffer& {
        std::swap(buffer, other.buffer);
        std::swap(allocation, other.allocation);
        std::swap(bufferSize, other.bufferSize);
        std::swap(allocator, other.allocator);
        return *this;
    }

    void update(const void* data, vk::DeviceSize dataSize);

    [[nodiscard]] auto get() const noexcept -> vk::Buffer;
};

} // namespace Vulkan
