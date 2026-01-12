#pragma once

#include "device.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {

class UploadContext;
class CommandPool;

class Image {
  private:
    const vk::raii::Device* device{};
    vk::Image image;
    vk::raii::ImageView imageView = nullptr;
    vk::Format imageFormat = vk::Format();
    vk::Extent3D imageExtent;
    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    vk::ImageUsageFlags usageFlags;
    vk::PipelineStageFlags2 stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    vk::AccessFlags2 accessFlags;
    uint32_t mipLevelCount = 1;

    const VmaAllocator* allocator{};
    VmaAllocation allocation{};

    friend class UploadContext;
    friend class CommandPool;

  public:
    Image() = default;
    // Constructor for swapchain images
    Image(const vk::raii::Device* device,
          vk::Image image,
          vk::Extent2D extent2D,
          vk::Format format,
          vk::ImageAspectFlags aspectFlags);
    // Constructor for owned images (textures, etc)
    Image(Vulkan::Device& device,
          vk::Extent3D extent,
          vk::Format format,
          vk::ImageUsageFlags usageFlags,
          vk::ImageAspectFlags aspectFlags,
          vk::SampleCountFlagBits samplesPerPixel = vk::SampleCountFlagBits::e1,
          uint32_t mipLevelCount = 1);

    ~Image() {
        if (allocation != nullptr) {
            vmaDestroyImage(*allocator, image, allocation);
        }
    }

    // Delete copy operations (can't copy Vulkan resources)
    Image(const Image&) = delete;
    auto operator=(const Image&) -> Image& = delete;

    // Move constructor
    Image(Image&& other) noexcept
        : device(other.device), allocator(other.allocator), image(std::exchange(other.image, nullptr)),
          imageView(std::move(other.imageView)), imageFormat(other.imageFormat), imageExtent(other.imageExtent),
          layout(other.layout), usageFlags(other.usageFlags), stageMask(other.stageMask),
          accessFlags(other.accessFlags), allocation(std::exchange(other.allocation, nullptr)),
          mipLevelCount(other.mipLevelCount) {}

    // Move assignment
    auto operator=(Image&& other) noexcept -> Image& {
        if (this != &other) {
            // Clean up existing resources
            if (allocation != nullptr) {
                vmaDestroyImage(*allocator, image, allocation);
            }

            // Move from other
            device = other.device;
            allocator = other.allocator;
            image = std::exchange(other.image, nullptr);
            imageView = std::move(other.imageView);
            imageFormat = other.imageFormat;
            imageExtent = other.imageExtent;
            layout = other.layout;
            usageFlags = other.usageFlags;
            stageMask = other.stageMask;
            accessFlags = other.accessFlags;
            allocation = std::exchange(other.allocation, nullptr);
            mipLevelCount = other.mipLevelCount;
        }
        return *this;
    }

    auto get() -> const vk::Image&;
    auto getImageView() -> const vk::raii::ImageView&;
};
} // namespace Vulkan