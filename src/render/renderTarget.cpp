#include "renderTarget.hpp"

Render::RenderTarget::RenderTarget(Vulkan::Image& msaaImage,
                                   Vulkan::Image& colorImage,
                                   Vulkan::Image& depthImage,
                                   vk::Extent2D extent,
                                   vk::Format format)
    : msaaImage(&msaaImage), colorImage(&colorImage), depthImage(&depthImage), extent(extent), format(format) {}

auto Render::RenderTarget::getMsaaImage() -> Vulkan::Image& { return *msaaImage; }

auto Render::RenderTarget::getColorImage() -> Vulkan::Image& { return *colorImage; }

auto Render::RenderTarget::getDepthImage() -> Vulkan::Image& { return *depthImage; }

auto Render::RenderTarget::getExtent() -> const vk::Extent2D& { return extent; }

auto Render::RenderTarget::getFormat() -> const vk::Format& { return format; }

auto Render::RenderTarget::getFramebuffer() -> const vk::raii::Framebuffer& { return framebuffer; }
