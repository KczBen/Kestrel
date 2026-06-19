#include "device.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "material.hpp"
#include "pass.hpp"
#include "physicalDevice.hpp"
#include "renderTarget.hpp"
#include "resourceManager.hpp"
#include "surface.hpp"
#include <gtest/gtest.h>

using namespace Vulkan;

class RenderTargetFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    ResourceManager resources{physicalDevice, device};
};

TEST_F(RenderTargetFixture, CreateRenderTargetCreatesValidImages) {
    auto handle = resources.createRenderTarget(
        RenderTarget::CreateInfo{
            .extent = vk::Extent2D{800, 600},
            .format = vk::Format::eB8G8R8A8Srgb,
            .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
        },
        vk::SampleCountFlagBits::e1);

    ASSERT_TRUE(handle.isPresent());
    EXPECT_TRUE(handle->colorImage != nullptr);
    EXPECT_TRUE(handle->depthImage != nullptr);
}

TEST_F(RenderTargetFixture, CreateRenderTargetCreatesDepthImage) {
    auto handle = resources.createRenderTarget(
        RenderTarget::CreateInfo{
            .extent = vk::Extent2D{800, 600},
            .format = vk::Format::eB8G8R8A8Srgb,
            .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
        },
        vk::SampleCountFlagBits::e1);

    auto& depthImage = handle->depthImage;
    auto imageView = depthImage->getImageView();
    ASSERT_NE(imageView.get(), VK_NULL_HANDLE);
}

TEST_F(RenderTargetFixture, MultipleTargetsAreDistinct) {
    auto handle0 = resources.createRenderTarget(
        RenderTarget::CreateInfo{
            .extent = vk::Extent2D{800, 600},
            .format = vk::Format::eB8G8R8A8Srgb,
            .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
        },
        vk::SampleCountFlagBits::e1);

    auto handle1 = resources.createRenderTarget(
        RenderTarget::CreateInfo{
            .extent = vk::Extent2D{800, 600},
            .format = vk::Format::eB8G8R8A8Srgb,
            .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
        },
        vk::SampleCountFlagBits::e1);

    ASSERT_TRUE(handle0.isPresent());
    ASSERT_TRUE(handle1.isPresent());
    ASSERT_NE(handle0, handle1);
    ASSERT_NE(handle0->colorImage, handle1->colorImage);
    ASSERT_NE(handle0->depthImage, handle1->depthImage);
}
