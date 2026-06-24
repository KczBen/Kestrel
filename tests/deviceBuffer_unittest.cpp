#include "device.hpp"
#include "deviceBuffer.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

using namespace Vulkan;

class DeviceBufferFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    DeviceBuffer buffer{device, 256, vk::BufferUsageFlagBits::eTransferSrc};
};

TEST_F(DeviceBufferFixture, GetReturnsNonNull) { ASSERT_NE(*buffer, VK_NULL_HANDLE); }

TEST_F(DeviceBufferFixture, BufferSizeReturnsCorrect) { EXPECT_EQ(buffer.size(), 256); }

TEST_F(DeviceBufferFixture, BufferSizeZero) {
    DeviceBuffer zeroBuffer{device, 0, vk::BufferUsageFlagBits::eIndexBuffer};
    EXPECT_EQ(*zeroBuffer, VK_NULL_HANDLE);
    EXPECT_EQ(zeroBuffer.size(), 0);
}

TEST_F(DeviceBufferFixture, MoveConstructor) {
    DeviceBuffer buffer1{device, 128, vk::BufferUsageFlagBits::eTransferDst};
    DeviceBuffer buffer2(std::move(buffer1));
    ASSERT_EQ(*buffer1, VK_NULL_HANDLE);
    ASSERT_NE(*buffer2, VK_NULL_HANDLE);
}

TEST_F(DeviceBufferFixture, MoveAssignment) {
    DeviceBuffer buffer1{device, 64, vk::BufferUsageFlagBits::eStorageBuffer};
    DeviceBuffer buffer2{device, 32, vk::BufferUsageFlagBits::eUniformBuffer};
    buffer2 = std::move(buffer1);
    ASSERT_EQ(*buffer1, VK_NULL_HANDLE);
    ASSERT_NE(*buffer2, VK_NULL_HANDLE);
}

TEST_F(DeviceBufferFixture, BufferUsageTransferSrc) {
    DeviceBuffer buffer{device, 1024, vk::BufferUsageFlagBits::eTransferSrc};
    ASSERT_NE(*buffer, VK_NULL_HANDLE);
    EXPECT_EQ(buffer.size(), 1024);
}

TEST_F(DeviceBufferFixture, BufferUsageStorageBuffer) {
    DeviceBuffer buffer{device, 512, vk::BufferUsageFlagBits::eStorageBuffer};
    ASSERT_NE(*buffer, VK_NULL_HANDLE);
    EXPECT_EQ(buffer.size(), 512);
}
