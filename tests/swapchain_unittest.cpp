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
    Swapchain swapchain{physicalDevice, device, surface, 800, 600};
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

// Cover the other constructor
TEST_F(SwapchainFixture, PairConstructorProducesValidSwapchain) {
    Swapchain sc{physicalDevice, device, surface, std::make_pair(800u, 600u)};
    ASSERT_NE(sc.get(), nullptr);
    EXPECT_GT(sc.getImages().size(), 0u);
    EXPECT_NE(sc.getExtent().width, 0u);
    EXPECT_NE(sc.getExtent().height, 0u);
}

TEST_F(SwapchainFixture, PresentModeIsSupportedByDevice) {
    const auto& modes = physicalDevice.getSwapChainSupportDetails().presentModes;
    const bool hasFifo = std::find(modes.begin(), modes.end(), vk::PresentModeKHR::eFifo) != modes.end();
    EXPECT_TRUE(hasFifo);
}

// Requestin a very large swapchain, createSwapchain clamps to maxImageCount when the driver reports one
TEST_F(SwapchainFixture, ImageCountRespectsCap) {
    const auto& caps = physicalDevice.getSwapChainSupportDetails().capabilities;
    // The actual image count must stay within driver-reported bounds
    EXPECT_GE(swapchain.getImages().size(), caps.minImageCount);
    if (caps.maxImageCount > 0) {
        EXPECT_LE(swapchain.getImages().size(), caps.maxImageCount);
    }
}

// Swapchain recreation
TEST_F(SwapchainFixture, RecreateUpdatesExtent) {
    swapchain.recreate(device, surface, 1024, 768);
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

// Recreate with the same dimensions, covers an additional branch
TEST_F(SwapchainFixture, RecreateSameSizeStillValid) {
    swapchain.recreate(device, surface, 800, 600);
    ASSERT_NE(swapchain.get(), nullptr);
    EXPECT_GT(swapchain.getImages().size(), 0u);
}