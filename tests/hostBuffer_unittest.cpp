#include "device.hpp"
#include "hostBuffer.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

using namespace Vulkan;

class HostBufferFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    HostBuffer buffer{device, 256};
};

TEST_F(HostBufferFixture, GetReturnsNonNull) { ASSERT_NE(buffer.get(), VK_NULL_HANDLE); }

TEST_F(HostBufferFixture, UpdateBuffer) {
    float data = 42.0f;
    buffer.update(&data, sizeof(float));
    SUCCEED();
}

TEST_F(HostBufferFixture, MoveConstructor) {
    HostBuffer buffer1{device, 128};
    HostBuffer buffer2(std::move(buffer1));
    ASSERT_EQ(buffer1.get(), VK_NULL_HANDLE);
    ASSERT_NE(buffer2.get(), VK_NULL_HANDLE);
}

TEST_F(HostBufferFixture, MoveAssignment) {
    HostBuffer buffer1{device, 64};
    HostBuffer buffer2{device, 32};
    buffer2 = std::move(buffer1);
    ASSERT_EQ(buffer1.get(), VK_NULL_HANDLE);
    ASSERT_NE(buffer2.get(), VK_NULL_HANDLE);
}

TEST_F(HostBufferFixture, UniformBufferUsage) {
    HostBuffer buffer{device, 64, vk::BufferUsageFlagBits::eUniformBuffer};
    ASSERT_NE(buffer.get(), VK_NULL_HANDLE);
}

TEST_F(HostBufferFixture, StorageBufferUsage) {
    HostBuffer buffer{device, 128, vk::BufferUsageFlagBits::eStorageBuffer};
    ASSERT_NE(buffer.get(), VK_NULL_HANDLE);
}
