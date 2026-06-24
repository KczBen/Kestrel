#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

using namespace Vulkan;

class PhysicalDeviceFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
};

TEST_F(PhysicalDeviceFixture, GetReturnsNonNull) { ASSERT_NE(*physicalDevice, nullptr); }

TEST_F(PhysicalDeviceFixture, ExtensionsNonEmpty) { ASSERT_FALSE(physicalDevice.getExtensions().empty()); }

TEST_F(PhysicalDeviceFixture, QueueFamilyIndicesPresent) {
    auto indices = physicalDevice.getQueueFamilyIndices();
    EXPECT_TRUE(indices.graphicsFamily.has_value());
    EXPECT_TRUE(indices.presentFamily.has_value());
    EXPECT_TRUE(indices.computeFamily.has_value());
}

TEST_F(PhysicalDeviceFixture, SwapChainSupportNonEmpty) {
    auto swapSupport = physicalDevice.getSwapChainSupportDetails();
    // All compliant devices with a swapchain should support at least one format and present mode
    EXPECT_FALSE(swapSupport.formats.empty());
    EXPECT_FALSE(swapSupport.presentModes.empty());
}

TEST_F(PhysicalDeviceFixture, SwapChainCapabilitiesValid) {
    const auto& caps = physicalDevice.getSwapChainSupportDetails().capabilities;
    // zero is a valid maxImageCount, see
    // https://docs.vulkan.org/refpages/latest/refpages/source/VkSurfaceCapabilitiesKHR.html
    if (caps.maxImageCount != 0) {
        EXPECT_GE(caps.maxImageCount, caps.minImageCount);
    }
    EXPECT_GE(caps.minImageExtent.width, 0u);
    EXPECT_GE(caps.minImageExtent.height, 0u);
}

TEST_F(PhysicalDeviceFixture, QueueFamilyIndicesAreValidIndices) {
    auto indices = physicalDevice.getQueueFamilyIndices();
    // Queue family indices must be non-negative integers; max reasonable value is 64
    EXPECT_LT(indices.graphicsFamily.value(), 64u);
    EXPECT_LT(indices.presentFamily.value(), 64u);
    EXPECT_LT(indices.computeFamily.value(), 64u);
}

TEST_F(PhysicalDeviceFixture, ExtensionsContainSwapchain) {
    const auto& exts = physicalDevice.getExtensions();
    const std::string_view swapchainExt = vk::KHRSwapchainExtensionName;
    bool found = false;
    for (const auto* ext : exts) {
        if (ext == swapchainExt) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// FIFO is guaranteed by spec on all Vulkan devices
TEST_F(PhysicalDeviceFixture, SwapChainSupportContainsFifo) {
    const auto& modes = physicalDevice.getSwapChainSupportDetails().presentModes;
    const bool hasFifo = std::ranges::find(modes, vk::PresentModeKHR::eFifo) != modes.end();
    EXPECT_TRUE(hasFifo);
}