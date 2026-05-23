#pragma once

#include "device.hpp"
#include "resourceHandles.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"
#include <deque>

namespace Vulkan {
class HostBuffer {
  private:
    VmaAllocator allocator;
    vk::Buffer buffer;
    VmaAllocation allocation{};
    void* mapped{};
    vk::DeviceSize bufferSize{};

  public:
    HostBuffer(Vulkan::Device& device,
               vk::DeviceSize size,
               vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eUniformBuffer);

    ~HostBuffer() {
        if (buffer) {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    }

    HostBuffer(const HostBuffer&) = delete;
    auto operator=(const HostBuffer&) -> HostBuffer& = delete;

    HostBuffer(HostBuffer&& other) noexcept
        : allocator(other.allocator), buffer(other.buffer), allocation(other.allocation), bufferSize(other.bufferSize),
          mapped(other.mapped) {
        other.buffer = VK_NULL_HANDLE;
        other.allocation = nullptr;
        other.bufferSize = 0;
        other.mapped = nullptr;
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

using HostBufferHandle = Render::ResourceHandle<HostBuffer>;

} // namespace Vulkan