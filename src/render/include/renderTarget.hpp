#pragma once

#include "image.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Render {
class RenderTarget {
  private:
    Vulkan::Image* msaaImage;
    Vulkan::Image* colorImage;
    Vulkan::Image* depthImage;
    vk::Extent2D extent;
    vk::Format format;
    vk::raii::Framebuffer framebuffer = nullptr;

  public:
    RenderTarget(Vulkan::Image& msaaImage,
                 Vulkan::Image& colorImage,
                 Vulkan::Image& depthImage,
                 vk::Extent2D extent,
                 vk::Format format);

    auto getMsaaImage() -> Vulkan::Image&;
    auto getColorImage() -> Vulkan::Image&;
    auto getDepthImage() -> Vulkan::Image&;
    auto getExtent() -> const vk::Extent2D&;
    auto getFormat() -> const vk::Format&;
    auto getFramebuffer() -> const vk::raii::Framebuffer&;
};
} // namespace Render
