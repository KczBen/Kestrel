#include "device.hpp"
#include "hostBuffer.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

using namespace Vulkan;

namespace {
struct TestBufferType {
    float data[64];
};
} // namespace

class HostBufferFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    HostBuffer<TestBufferType> buffer{device};
};

TEST_F(HostBufferFixture, GetReturnsNonNull) { ASSERT_TRUE(static_cast<bool>(buffer.get())); }

TEST_F(HostBufferFixture, UpdateBuffer) {
    float data = 42.0f;
    buffer.update(&data, sizeof(float));
    SUCCEED();
}

TEST_F(HostBufferFixture, MoveConstructor) {
    HostBuffer<TestBufferType> buffer1{device};
    HostBuffer<TestBufferType> buffer2(std::move(buffer1));
    ASSERT_FALSE(static_cast<bool>(buffer1.get()));
    ASSERT_TRUE(static_cast<bool>(buffer2.get()));
}

TEST_F(HostBufferFixture, MoveAssignment) {
    HostBuffer<TestBufferType> buffer1{device};
    HostBuffer<TestBufferType> buffer2{device};
    buffer2 = std::move(buffer1);
    ASSERT_FALSE(static_cast<bool>(buffer1.get()));
    ASSERT_TRUE(static_cast<bool>(buffer2.get()));
}

TEST_F(HostBufferFixture, UniformBufferUsage) {
    HostBuffer<TestBufferType> buffer{device, vk::BufferUsageFlagBits::eUniformBuffer};
    ASSERT_TRUE(static_cast<bool>(buffer.get()));
}

TEST_F(HostBufferFixture, StorageBufferUsage) {
    HostBuffer<TestBufferType> buffer{device, vk::BufferUsageFlagBits::eStorageBuffer};
    ASSERT_TRUE(static_cast<bool>(buffer.get()));
}
