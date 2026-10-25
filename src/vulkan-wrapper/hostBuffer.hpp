#pragma once

#include "device.hpp"
#include "render/resourceHandles.hpp"
#include "util/gpuDataObjects.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Vulkan {
template <typename T> class HostBuffer {
  private:
    VmaAllocator allocator;
    vk::Buffer buffer;
    VmaAllocation allocation{};
    void* mapped{};
    vk::DeviceSize bufferSize{};

  public:
    explicit HostBuffer(Vulkan::Device& device, vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eUniformBuffer)
        : allocator(device.getAllocator()), bufferSize(sizeof(T)) {
        vk::BufferCreateInfo bufferInfo({}, bufferSize, usage);

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo info{};
        VkBuffer raw{};
        vmaCreateBuffer(
            allocator, &static_cast<const VkBufferCreateInfo&>(bufferInfo), &allocInfo, &raw, &allocation, &info);

        buffer = raw;
        mapped = info.pMappedData;
    }

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
        if (this != &other) {
            if (buffer) {
                vmaDestroyBuffer(allocator, buffer, allocation);
            }
            allocator = other.allocator;
            buffer = other.buffer;
            allocation = other.allocation;
            bufferSize = other.bufferSize;
            mapped = other.mapped;
            other.buffer = VK_NULL_HANDLE;
            other.allocation = nullptr;
            other.bufferSize = 0;
            other.mapped = nullptr;
        }
        return *this;
    }

    void update(const void* data, vk::DeviceSize dataSize) { std::memcpy(mapped, data, dataSize); };

    [[nodiscard]] auto data() noexcept -> T* { return static_cast<T*>(mapped); }
    [[nodiscard]] auto data() const noexcept -> const T* { return static_cast<const T*>(mapped); }

    [[nodiscard]] auto operator->() noexcept -> T* { return data(); }
    [[nodiscard]] auto operator*() noexcept -> T& { return *data(); }

    [[nodiscard]] auto get() const noexcept -> vk::Buffer { return buffer; }
    [[nodiscard]] auto byteSize() const noexcept -> vk::DeviceSize { return bufferSize; }
};

using MaterialUBOHandle = Render::ResourceHandle<HostBuffer<MaterialUBO>>;
} // namespace Vulkan