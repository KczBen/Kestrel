#include "commandPool.hpp"
#include "device.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include <gtest/gtest.h>

using namespace Vulkan;

static constexpr uint32_t maxFramesInFlight = 2;

class CommandPoolFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    CommandPool commandPool{physicalDevice, device, maxFramesInFlight};
};

TEST_F(CommandPoolFixture, GetReturnsNonNull) { ASSERT_NE(*commandPool.get(), VK_NULL_HANDLE); }

TEST_F(CommandPoolFixture, GetCommandBufferReturnsNonNullForAllFrames) {
    for (uint32_t frame = 0; frame < maxFramesInFlight; ++frame) {
        EXPECT_NE(*commandPool.getCommandBuffer(frame), VK_NULL_HANDLE) << "Frame index: " << frame;
    }
}

TEST_F(CommandPoolFixture, GetCommandBufferReturnsDistinctBuffersPerFrame) {
    const auto& buf0 = commandPool.getCommandBuffer(0);
    const auto& buf1 = commandPool.getCommandBuffer(1);
    EXPECT_NE(*buf0, *buf1);
}
