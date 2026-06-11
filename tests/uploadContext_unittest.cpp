#include "commandPool.hpp"
#include "device.hpp"
#include "deviceBuffer.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include "uploadBuffer.hpp"
#include "uploadContext.hpp"
#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

using namespace Vulkan;

static constexpr uint32_t maxFramesInFlight = 2;

class UploadContextFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    CommandPool commandPool{physicalDevice, device, maxFramesInFlight};
    UploadBuffer staging{device, 4096};
    UploadContext context{device, commandPool, staging};
};

TEST_F(UploadContextFixture, ContextCreated) { SUCCEED(); }

TEST_F(UploadContextFixture, UploadSmallBuffer) {
    DeviceBuffer dst{device, 64, vk::BufferUsageFlagBits::eStorageBuffer};
    const char data[] = "hello";
    context.upload(dst, data, 6, 0);
    SUCCEED();
}

TEST_F(UploadContextFixture, UploadZeroSizeBuffer) {
    DeviceBuffer dst{device, 0, vk::BufferUsageFlagBits::eStorageBuffer};
    const char data[] = "test";
    context.upload(dst, data, 0, 0);
    SUCCEED();
}

TEST_F(UploadContextFixture, UploadDifferentOffsets) {
    DeviceBuffer dst{device, 256, vk::BufferUsageFlagBits::eStorageBuffer};
    const char data[] = "test";
    context.upload(dst, data, 4, 128);
    SUCCEED();
}
