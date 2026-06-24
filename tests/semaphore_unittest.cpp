#include "device.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "semaphore.hpp"
#include "surface.hpp"
#include <gtest/gtest.h>

using namespace Vulkan;

class SemaphoreFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    Semaphore semaphore{device};
};

TEST_F(SemaphoreFixture, GetReturnsNonNull) { ASSERT_NE(*semaphore, VK_NULL_HANDLE); }
