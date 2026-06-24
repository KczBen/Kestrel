#include "device.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include "uploadBuffer.hpp"
#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

using namespace Vulkan;

class UploadBufferFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    UploadBuffer buffer{device, 4096};
};

TEST_F(UploadBufferFixture, GetReturnsNonNull) { ASSERT_NE(*buffer, VK_NULL_HANDLE); }

TEST_F(UploadBufferFixture, AllocateSmallBuffer) {
    const auto alloc = buffer.allocate(64);
    EXPECT_EQ(alloc.offset, 0);
    EXPECT_EQ(alloc.size, 64);
    EXPECT_NE(alloc.cpuPtr, nullptr);
}

TEST_F(UploadBufferFixture, AllocateMultipleBuffers) {
    const auto alloc1 = buffer.allocate(64);
    const auto alloc2 = buffer.allocate(64);
    EXPECT_LT(alloc1.offset, alloc2.offset);
    EXPECT_EQ(alloc1.size, 64);
    EXPECT_EQ(alloc2.size, 64);
}

TEST_F(UploadBufferFixture, AllocateAligned) {
    const auto alloc1 = buffer.allocate(16);
    const auto alloc2 = buffer.allocate(16);
    // Second allocation should be 16-aligned
    EXPECT_EQ(alloc2.offset % 16, 0);
}

TEST_F(UploadBufferFixture, ResetResetsHead) {
    buffer.allocate(128);
    buffer.reset();
    const auto alloc = buffer.allocate(32);
    EXPECT_EQ(alloc.offset, 0);
}

TEST_F(UploadBufferFixture, MoveConstructor) {
    UploadBuffer buffer1{device, 1024};
    UploadBuffer buffer2(std::move(buffer1));
    ASSERT_EQ(*buffer1, VK_NULL_HANDLE);
    ASSERT_NE(*buffer2, VK_NULL_HANDLE);
}

TEST_F(UploadBufferFixture, MoveAssignment) {
    UploadBuffer buffer1{device, 512};
    UploadBuffer buffer2{device, 256};
    buffer2 = std::move(buffer1);
    ASSERT_EQ(*buffer1, VK_NULL_HANDLE);
    ASSERT_NE(*buffer2, VK_NULL_HANDLE);
}

TEST_F(UploadBufferFixture, OutOfSpaceThrows) {
    UploadBuffer smallBuffer{device, 16};
    smallBuffer.allocate(16);
    EXPECT_THROW(smallBuffer.allocate(16), std::runtime_error);
}
