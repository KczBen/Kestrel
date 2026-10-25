#pragma once

#include "commandPool.hpp"
#include "device.hpp"
#include "deviceBuffer.hpp"
#include "image.hpp"
#include "texture.hpp"
#include "uploadBuffer.hpp"
#include "vulkan/vulkan_raii.hpp"

namespace Vulkan {
class UploadContext {
  private:
    const vk::raii::Device* device;
    const vk::raii::Queue* queue;
    const vk::raii::CommandPool* pool;
    vk::raii::CommandBuffer cmd = nullptr;
    Vulkan::UploadBuffer* staging;

    void flushAndWait();

    // void transitionImage(Vulkan::Image& image,
    //                      vk::ImageLayout newLayout,
    //                      vk::PipelineStageFlags2 dstStage,
    //                      vk::AccessFlags2 dstAccess);

  public:
    UploadContext(Vulkan::Device& device, Vulkan::CommandPool& pool, Vulkan::UploadBuffer& staging);

    void upload(Vulkan::DeviceBuffer& dst, const void* data, vk::DeviceSize size, vk::DeviceSize offset);
    void upload(Vulkan::Texture& texture);

    ~UploadContext() noexcept { flushAndWait(); }

    UploadContext(const UploadContext&) = delete;
    auto operator=(const UploadContext&) -> UploadContext& = delete;

    UploadContext(UploadContext&&) = delete;
    auto operator=(UploadContext&&) -> UploadContext& = delete;
};
} // namespace Vulkan