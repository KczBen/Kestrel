#pragma once

#include <vulkan/vulkan.hpp>

namespace Vulkan {
class Image;
} // namespace Vulkan

namespace Render {

struct RenderTargetInfo {
    Vulkan::Image* colorImage;
    Vulkan::Image* depthImage;
};

struct RenderTargetCreateInfo {
    vk::Extent2D extent;
    vk::Format format;
    vk::ImageUsageFlags usage;
    vk::ImageAspectFlagBits aspect = vk::ImageAspectFlagBits::eColor;
};

} // namespace Render
