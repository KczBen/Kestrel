#include "hostBuffer.hpp"

Vulkan::HostBuffer::HostBuffer(Vulkan::Device& device, vk::DeviceSize size)
    : allocator(device.getAllocator()), bufferSize(size) {
    vk::BufferCreateInfo bufferInfo({}, size, vk::BufferUsageFlagBits::eUniformBuffer);

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

void Vulkan::HostBuffer::update(const void* data, vk::DeviceSize dataSize) { std::memcpy(mapped, data, dataSize); }

[[nodiscard]] auto Vulkan::HostBuffer::get() const noexcept -> vk::Buffer { return buffer; }