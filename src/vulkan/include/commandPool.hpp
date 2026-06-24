#pragma once

#include "device.hpp"
#include "image.hpp"
#include "physicalDevice.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class CommandPool {
  private:
    const vk::raii::Device* device;
    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::CommandPool pool = nullptr;
    std::vector<vk::raii::CommandBuffer> buffers;

  public:
    CommandPool(Vulkan::PhysicalDevice& physicalDevice, Vulkan::Device& device, uint32_t maxFramesInFlight);
    static void transitionImage(Image& image,
                                vk::ImageLayout newLayout,
                                vk::PipelineStageFlags2 dstStage,
                                vk::AccessFlags2 dstAccess,
                                vk::ImageAspectFlags dstAspect,
                                vk::CommandBuffer cmd);

    static void generateMipmaps(Vulkan::Image& image, vk::CommandBuffer cmd);
    auto getCommandBuffer(uint32_t frame) -> const vk::raii::CommandBuffer&;
    [[nodiscard]] auto operator*() const -> const vk::raii::CommandPool& { return pool; }
    [[nodiscard]] auto operator->() const -> const vk::raii::CommandPool* { return &pool; }
    auto get() -> const vk::raii::CommandPool&;
};
} // namespace Vulkan