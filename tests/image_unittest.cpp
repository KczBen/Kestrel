#include "device.hpp"
#include "image.hpp"
#include "vulkan/vulkan.hpp"
#include <gtest/gtest.h>
#include <vk_mem_alloc.h>

using namespace Vulkan;

class ImageFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
};

TEST_F(ImageFixture, ConstructSwapchainImage) {
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkImage vkImage{};
    VmaAllocation allocation{};
    vk::ImageCreateInfo imageInfo{{},
                                  vk::ImageType::e2D,
                                  vk::Format::eR8G8B8A8Unorm,
                                  vk::Extent3D{256, 256, 1},
                                  1,
                                  1,
                                  vk::SampleCountFlagBits::e1,
                                  vk::ImageTiling::eOptimal,
                                  vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst};

    vmaCreateImage(device.getAllocator(), imageInfo, &allocInfo, &vkImage, &allocation, nullptr);

    {
        Image img{&device.get(), vkImage, {256, 256}, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor};
        ASSERT_NE(img.get(), VK_NULL_HANDLE);
        ASSERT_NE(img.getImageView(), VK_NULL_HANDLE);
    }

    vmaDestroyImage(device.getAllocator(), vkImage, allocation);
}

TEST_F(ImageFixture, ConstructOwnedImage) {
    Image img{device,
              {256, 256, 1},
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
              vk::ImageAspectFlagBits::eColor};
    ASSERT_NE(img.get(), VK_NULL_HANDLE);
    ASSERT_NE(img.getImageView(), VK_NULL_HANDLE);
}

// Hits the mipLevelCount != 1 branch in the owned image constructor
TEST_F(ImageFixture, ConstructOwnedImageWithMipmaps) {
    Image img{device,
              {256, 256, 1},
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc |
                  vk::ImageUsageFlagBits::eTransferDst,
              vk::ImageAspectFlagBits::eColor,
              vk::SampleCountFlagBits::e1,
              2 /* any value != 1 triggers mip calculation */};
    ASSERT_NE(img.get(), VK_NULL_HANDLE);
    ASSERT_NE(img.getImageView(), VK_NULL_HANDLE);
}

TEST_F(ImageFixture, MoveConstructor) {
    Image img{device,
              {256, 256, 1},
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
              vk::ImageAspectFlagBits::eColor};
    Image movedImg(std::move(img));
    ASSERT_EQ(img.get(), VK_NULL_HANDLE);
    ASSERT_EQ(img.getImageView(), VK_NULL_HANDLE);
    ASSERT_NE(movedImg.get(), VK_NULL_HANDLE);
    ASSERT_NE(movedImg.getImageView(), VK_NULL_HANDLE);
}

TEST_F(ImageFixture, MoveAssignment) {
    Image img{device,
              {256, 256, 1},
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
              vk::ImageAspectFlagBits::eColor};
    Image movedImg{device,
                   {256, 256, 1},
                   vk::Format::eR8G8B8A8Srgb,
                   vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
                   vk::ImageAspectFlagBits::eColor};
    movedImg = std::move(img);
    ASSERT_EQ(img.get(), VK_NULL_HANDLE);
    ASSERT_EQ(img.getImageView(), VK_NULL_HANDLE);
    ASSERT_NE(movedImg.get(), VK_NULL_HANDLE);
    ASSERT_NE(movedImg.getImageView(), VK_NULL_HANDLE);
}

// Exercises move-assigning into a default-constructed (null) Image,
// ensuring the destructor's allocation == nullptr guard is exercised
// on the destination side.
TEST_F(ImageFixture, MoveAssignmentIntoDefault) {
    Image img{device,
              {256, 256, 1},
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
              vk::ImageAspectFlagBits::eColor};
    Image defaultImg{};
    defaultImg = std::move(img);
    ASSERT_EQ(img.get(), VK_NULL_HANDLE);
    ASSERT_NE(defaultImg.get(), VK_NULL_HANDLE);
}