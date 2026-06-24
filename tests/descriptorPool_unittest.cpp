#include "descriptorPool.hpp"
#include "device.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include <gtest/gtest.h>

using namespace Vulkan;

class DescriptorPoolFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    DescriptorPool descriptorPool{device};
};

TEST_F(DescriptorPoolFixture, GetReturnsNonNull) { ASSERT_NE(*descriptorPool, VK_NULL_HANDLE); }
