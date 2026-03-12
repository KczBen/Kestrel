#include "device.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include "swapchain.hpp"
#include "vulkan/vulkan.hpp"
#include <gtest/gtest.h>

using namespace Vulkan;

class SwapchainFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    Swapchain swapchain{physicalDevice, device, surface, 800, 600, vk::SampleCountFlagBits::e4};
};

TEST_F(SwapchainFixture, GetReturnsNonNull) { ASSERT_NE(swapchain.get(), nullptr); }

TEST_F(SwapchainFixture, FormatReturnsCorrect) {
    // Check that the format returned by the swapchain is one of the supported formats reported by the physical device
    // We can't just check for one specific format, as it is not defined
    const auto& support = physicalDevice.getSwapChainSupportDetails();
    bool supported = false;
    for (const auto& fmt : support.formats) {
        if (fmt.format == swapchain.getFormat()) {
            supported = true;
            break;
        }
    }
    ASSERT_TRUE(supported);
}

TEST_F(SwapchainFixture, ExtentNotZero) {
    const auto& extent = swapchain.getExtent();
    EXPECT_NE(extent.width, 0u);
    EXPECT_NE(extent.height, 0u);
}

TEST_F(SwapchainFixture, ImagesNotEmpty) { EXPECT_GT(swapchain.getImages().size(), 0u); }

TEST_F(SwapchainFixture, GetImageReturnsValid) {
    auto& img = swapchain.getImage(0);
    // image object should not be empty; check its image view not null
    ASSERT_NE(img.getImageView(), nullptr);
}

TEST_F(SwapchainFixture, GetDepthImageReturnsValid) {
    auto& depth = swapchain.getDepthImage();
    ASSERT_NE(depth.getImageView(), nullptr);
}

TEST_F(SwapchainFixture, GetMsaaImageReturnsValid) {
    auto& msaa = swapchain.getMsaaImage();
    ASSERT_NE(msaa.getImageView(), nullptr);
}

TEST_F(SwapchainFixture, MsaaSampleCountMatches) {
    EXPECT_EQ(swapchain.getMsaaSampleCount(), vk::SampleCountFlagBits::e4);
}

// Swapchain recreation
TEST_F(SwapchainFixture, RecreateUpdatesExtent) {
    const uint32_t newWidth = 1024;
    const uint32_t newHeight = 768;
    swapchain.recreate(device, surface, newWidth, newHeight);
    const auto& extent = swapchain.getExtent();
    EXPECT_GT(extent.width, 0u);
    EXPECT_GT(extent.height, 0u);
}

TEST_F(SwapchainFixture, RecreateImagesNotEmpty) {
    swapchain.recreate(device, surface, 1024, 768);
    EXPECT_GT(swapchain.getImages().size(), 0u);
}

TEST_F(SwapchainFixture, RecreateGetImageReturnsValid) {
    swapchain.recreate(device, surface, 1024, 768);
    auto& img = swapchain.getImage(0);
    ASSERT_NE(img.getImageView(), nullptr);
}

TEST_F(SwapchainFixture, RecreateGetDepthImageReturnsValid) {
    swapchain.recreate(device, surface, 1024, 768);
    auto& depth = swapchain.getDepthImage();
    ASSERT_NE(depth.getImageView(), nullptr);
}

TEST_F(SwapchainFixture, RecreateGetMsaaImageReturnsValid) {
    swapchain.recreate(device, surface, 1024, 768);
    auto& msaa = swapchain.getMsaaImage();
    ASSERT_NE(msaa.getImageView(), nullptr);
}

TEST_F(SwapchainFixture, RecreateMsaaSampleCountMatches) {
    swapchain.recreate(device, surface, 1024, 768);
    EXPECT_EQ(swapchain.getMsaaSampleCount(), vk::SampleCountFlagBits::e4);
}

TEST_F(SwapchainFixture, RecreateFormatReturnsCorrect) {
    swapchain.recreate(device, surface, 1024, 768);
    const auto& support = physicalDevice.getSwapChainSupportDetails();
    bool supported = false;
    for (const auto& fmt : support.formats) {
        if (fmt.format == swapchain.getFormat()) {
            supported = true;
            break;
        }
    }
    ASSERT_TRUE(supported);
}

TEST_F(SwapchainFixture, RecreateReturnsNonNull) {
    swapchain.recreate(device, surface, 1024, 768);
    ASSERT_NE(swapchain.get(), nullptr);
}
