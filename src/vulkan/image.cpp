#include "image.hpp"
#include <cmath>
#include <vulkan/vulkan_raii.hpp>

Vulkan::Image::Image(const vk::raii::Device* device,
                     vk::Image image,
                     vk::Extent2D extent2D,
                     vk::Format format,
                     vk::ImageAspectFlags aspectFlags)
    : device(device), image(image), imageExtent(extent2D.width, extent2D.height, 1), imageFormat(format) {
    const vk::ImageViewCreateInfo cInfo(vk::ImageViewCreateFlags(),
                                        this->image,
                                        vk::ImageViewType::e2D,
                                        this->imageFormat,
                                        vk::ComponentMapping(),
                                        vk::ImageSubresourceRange(aspectFlags, 0, 1, 0, 1));

    this->imageView = vk::raii::ImageView(*device, cInfo);
}

Vulkan::Image::Image(Vulkan::Device& device,
                     vk::Extent3D extent,
                     vk::Format format,
                     vk::ImageUsageFlags usageFlags,
                     vk::ImageAspectFlags aspectFlags,
                     vk::SampleCountFlagBits samplesPerPixel,
                     uint32_t mipLevelCount)
    : imageExtent(extent), imageFormat(format), usageFlags(usageFlags), allocator(&device.getAllocator()) {

    if (mipLevelCount != 1) {
        mipLevelCount = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height))) + 1);
    }

    const vk::ImageCreateInfo imageInfo({},
                                        vk::ImageType::e2D,
                                        format,
                                        extent,
                                        mipLevelCount,
                                        1,
                                        samplesPerPixel,
                                        vk::ImageTiling::eOptimal,
                                        usageFlags,
                                        vk::SharingMode::eExclusive);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage vkImage{};
    vmaCreateImage(device.getAllocator(), imageInfo, &allocInfo, &vkImage, &allocation, nullptr);

    image = vkImage;

    const vk::ImageViewCreateInfo viewInfo(
        {}, image, vk::ImageViewType::e2D, format, {}, vk::ImageSubresourceRange(aspectFlags, 0, mipLevelCount, 0, 1));

    imageView = vk::raii::ImageView(device.get(), viewInfo);
}

auto Vulkan::Image::get() -> const vk::Image& { return image; }
auto Vulkan::Image::getImageView() -> const vk::raii::ImageView& { return imageView; }