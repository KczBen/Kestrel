#include "deviceBuffer.hpp"

Vulkan::DeviceBuffer::DeviceBuffer(Vulkan::Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage)
    : allocator(device.getAllocator()), bufferSize(size) {
    vk::BufferCreateInfo bufferInfo({}, size, usage);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VkBuffer raw{};
    vmaCreateBuffer(
        allocator, &static_cast<const VkBufferCreateInfo&>(bufferInfo), &allocInfo, &raw, &allocation, nullptr);

    buffer = raw;
}

auto Vulkan::DeviceBuffer::get() const noexcept -> vk::Buffer { return buffer; }
auto Vulkan::DeviceBuffer::size() const noexcept -> vk::DeviceSize { return bufferSize; }