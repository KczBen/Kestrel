#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
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

TEST_F(PhysicalDeviceFixture, GetReturnsNonNull) { ASSERT_NE(physicalDevice.get(), nullptr); }

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