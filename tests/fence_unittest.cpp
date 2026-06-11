#include "device.hpp"
#include "fence.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include <gtest/gtest.h>

using namespace Vulkan;

class FenceFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    vk::FenceCreateFlags flags;
    Fence fence{device, flags};
};

TEST_F(FenceFixture, GetReturnsNonNull) { ASSERT_NE(*fence.get(), VK_NULL_HANDLE); }

