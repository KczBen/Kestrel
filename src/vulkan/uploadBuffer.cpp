#include "uploadBuffer.hpp"
#include "device.hpp"

Vulkan::UploadBuffer::UploadBuffer(Vulkan::Device& device, vk::DeviceSize bufferSize)
    : allocator(device.getAllocator()), capacity(bufferSize) {
    vk::BufferCreateInfo bufferInfo({}, bufferSize, vk::BufferUsageFlagBits::eTransferSrc);

    const VmaAllocationCreateInfo allocInfo{.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                     VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                            .usage = VMA_MEMORY_USAGE_AUTO};

    VmaAllocationInfo info{};
    VkBuffer raw{};
    vmaCreateBuffer(
        allocator, &static_cast<const VkBufferCreateInfo&>(bufferInfo), &allocInfo, &raw, &allocation, &info);

    buffer = raw;
    mapped = info.pMappedData;
}

auto Vulkan::UploadBuffer::allocate(vk::DeviceSize size, vk::DeviceSize alignment) -> Allocation {
    const vk::DeviceSize aligned = (head + alignment - 1) & ~(alignment - 1);

    if (aligned + size > capacity) {
        throw std::runtime_error("UploadBuffer out of space");
    }

    Allocation alloc{.offset = aligned, .size = size, .cpuPtr = static_cast<char*>(mapped) + aligned};

    head = aligned + size;
    return alloc;
}

void Vulkan::UploadBuffer::reset() { head = 0; }

[[nodiscard]] auto Vulkan::UploadBuffer::get() const noexcept -> vk::Buffer { return buffer; }