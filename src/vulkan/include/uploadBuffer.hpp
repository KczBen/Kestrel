#pragma once

#include "device.hpp"
#include "vk_mem_alloc.h"

namespace Vulkan {
class UploadBuffer {
  private:
    VmaAllocator allocator{};
    vk::Buffer buffer;
    VmaAllocation allocation{};
    void* mapped{};
    vk::DeviceSize capacity{};
    vk::DeviceSize head{};

    void cleanup() {
        if (buffer) {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    }

  public:
    struct Allocation {
        vk::DeviceSize offset;
        vk::DeviceSize size;
        void* cpuPtr;
    };

    UploadBuffer(Vulkan::Device& device, vk::DeviceSize bufferSize);

    ~UploadBuffer() { cleanup(); }

    UploadBuffer(const UploadBuffer&) = delete;
    auto operator=(const UploadBuffer&) -> UploadBuffer& = delete;

    UploadBuffer(UploadBuffer&& other) noexcept
        : allocator(other.allocator), buffer(other.buffer), allocation(other.allocation), mapped(other.mapped),
          capacity(other.capacity), head(other.head) {
        other.buffer = VK_NULL_HANDLE;
        other.allocation = nullptr;
        other.mapped = nullptr;
        other.capacity = 0;
        other.head = 0;
    }

    auto operator=(UploadBuffer&& other) noexcept -> UploadBuffer& {
        if (this != &other) {
            cleanup();

            allocator = other.allocator;
            buffer = other.buffer;
            allocation = other.allocation;
            mapped = other.mapped;
            capacity = other.capacity;
            head = other.head;

            other.buffer = VK_NULL_HANDLE;
            other.allocation = nullptr;
            other.mapped = nullptr;
            other.capacity = 0;
            other.head = 0;
        }
        return *this;
    }

    auto allocate(vk::DeviceSize size, vk::DeviceSize alignment = 16) -> Allocation;

    void reset();

    [[nodiscard]] auto operator*() const noexcept -> vk::Buffer { return buffer; }
    [[nodiscard]] auto get() const noexcept -> vk::Buffer;
};
} // namespace Vulkan