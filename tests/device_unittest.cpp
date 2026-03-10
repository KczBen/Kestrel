#include "device.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

using namespace Vulkan;

class DeviceFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
};

TEST_F(DeviceFixture, GetReturnsNonNull) { ASSERT_NE(device.get(), nullptr); }

TEST_F(DeviceFixture, PhysicalDeviceHandleNonNull) { ASSERT_NE(device.getPhysicalDevice(), nullptr); }

TEST_F(DeviceFixture, GraphicsQueueNonNull) { ASSERT_NE(device.getGraphicsQueue(), nullptr); }

TEST_F(DeviceFixture, PresentQueueNonNull) { ASSERT_NE(device.getPresentQueue(), nullptr); }

TEST_F(DeviceFixture, AllocatorNonNull) { ASSERT_NE(device.getAllocator(), nullptr); }