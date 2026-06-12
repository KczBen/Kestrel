#include "device.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "renderTarget.hpp"
#include "surface.hpp"
#include "swapchain.hpp"
#include <glm/vec2.hpp>
#include <gtest/gtest.h>

using namespace Vulkan;

class RenderTargetFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    Swapchain swapchain{physicalDevice, device, surface, 256, 256, vk::SampleCountFlagBits::e4};
    Render::RenderTarget target{swapchain.getMsaaImage(),
                                swapchain.getImages()[0],
                                swapchain.getDepthImage(),
                                swapchain.getExtent(),
                                swapchain.getFormat()};
};

TEST_F(RenderTargetFixture, GetMsaaImageValid) {
    auto& msaa = target.getMsaaImage();
    ASSERT_NE(msaa.getImageView(), VK_NULL_HANDLE);
}

TEST_F(RenderTargetFixture, GetColorImageValid) {
    auto& color = target.getColorImage();
    ASSERT_NE(color.getImageView(), VK_NULL_HANDLE);
}

TEST_F(RenderTargetFixture, GetDepthImageValid) {
    auto& depth = target.getDepthImage();
    ASSERT_NE(depth.getImageView(), VK_NULL_HANDLE);
}

TEST_F(RenderTargetFixture, GetExtentCorrect) {
    const auto& extent = target.getExtent();
    EXPECT_EQ(extent.width, 256u);
    EXPECT_EQ(extent.height, 256u);
}

TEST_F(RenderTargetFixture, GetFormatCorrect) { EXPECT_EQ(target.getFormat(), swapchain.getFormat()); }
