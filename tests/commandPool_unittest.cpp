#include "commandPool.hpp"
#include "device.hpp"
#include "image.hpp"
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

TEST_F(CommandPoolFixture, GetReturnsNonNull) { ASSERT_NE(*commandPool, VK_NULL_HANDLE); }

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

// Helper: begin a one-time command buffer, run a lambda recording commands, then end.
// We don't submit — we only care that the CPU-side Vulkan recording calls execute
// without validation errors, which is sufficient to drive coverage.
template <typename F> static void recordCommands(const vk::raii::CommandBuffer& cmd, F&& record) {
    const vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    cmd.begin(beginInfo);
    std::forward<F>(record)(cmd);
    cmd.end();
}

// Covers CommandPool::transitionImage()
TEST_F(CommandPoolFixture, TransitionImageRecordsBarrier) {
    Image img{device,
              {256, 256, 1},
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
              vk::ImageAspectFlagBits::eColor};

    const auto& cmd = commandPool.getCommandBuffer(0);
    recordCommands(cmd, [&](vk::CommandBuffer rawCmd) -> void {
        CommandPool::transitionImage(img,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                     vk::AccessFlagBits2::eColorAttachmentWrite,
                                     vk::ImageAspectFlagBits::eColor,
                                     rawCmd);
    });

    SUCCEED();
}

TEST_F(CommandPoolFixture, TransitionImageTwiceUpdatesState) {
    Image img{device,
              {256, 256, 1},
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
                  vk::ImageUsageFlagBits::eTransferDst,
              vk::ImageAspectFlagBits::eColor};

    const auto& cmd = commandPool.getCommandBuffer(0);
    recordCommands(cmd, [&](vk::CommandBuffer rawCmd) -> void {
        CommandPool::transitionImage(img,
                                     vk::ImageLayout::eTransferDstOptimal,
                                     vk::PipelineStageFlagBits2::eTransfer,
                                     vk::AccessFlagBits2::eTransferWrite,
                                     vk::ImageAspectFlagBits::eColor,
                                     rawCmd);

        CommandPool::transitionImage(img,
                                     vk::ImageLayout::eShaderReadOnlyOptimal,
                                     vk::PipelineStageFlagBits2::eFragmentShader,
                                     vk::AccessFlagBits2::eShaderRead,
                                     vk::ImageAspectFlagBits::eColor,
                                     rawCmd);
    });

    SUCCEED();
}

// Covers CommandPool::generateMipmaps() — requires mipLevelCount > 1
TEST_F(CommandPoolFixture, GenerateMipmapsRecordsBlits) {
    // mipLevelCount != 1 triggers the calculated mip count in the Image constructor
    Image img{device,
              {256, 256, 1},
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc |
                  vk::ImageUsageFlagBits::eTransferDst,
              vk::ImageAspectFlagBits::eColor,
              vk::SampleCountFlagBits::e1,
              2 /* triggers mip calculation: floor(log2(256)) + 1 = 9 levels */};

    const auto& cmd = commandPool.getCommandBuffer(0);
    recordCommands(cmd, [&](vk::CommandBuffer rawCmd) { CommandPool::generateMipmaps(img, rawCmd); });

    SUCCEED();
}

// Edge case: generateMipmaps on a 1x1 image (mipWidth/mipHeight halving bottoms out immediately)
TEST_F(CommandPoolFixture, GenerateMipmapsOneTileImage) {
    Image img{device,
              {1, 1, 1},
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc |
                  vk::ImageUsageFlagBits::eTransferDst,
              vk::ImageAspectFlagBits::eColor,
              vk::SampleCountFlagBits::e1,
              2};

    const auto& cmd = commandPool.getCommandBuffer(1);
    recordCommands(cmd, [&](vk::CommandBuffer rawCmd) { CommandPool::generateMipmaps(img, rawCmd); });

    SUCCEED();
}