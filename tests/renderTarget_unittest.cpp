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
using namespace Render;

class RenderTargetFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
    ResourceManager resources{physicalDevice, device};
};

TEST_F(RenderTargetFixture, CreateRenderTargetCreatesValidImage) {
    auto handle = resources.createRenderTarget(vk::Extent3D{800, 600, 1},
                                               vk::Format::eB8G8R8A8Srgb,
                                               vk::ImageUsageFlagBits::eColorAttachment |
                                                   vk::ImageUsageFlagBits::eTransientAttachment,
                                               vk::ImageAspectFlagBits::eColor);

    ASSERT_TRUE(handle.isPresent());
    EXPECT_NE(&*handle, static_cast<RenderTarget*>(nullptr));
}

TEST_F(RenderTargetFixture, CreateDepthRenderTarget) {
    auto handle = resources.createRenderTarget(vk::Extent3D{800, 600, 1},
                                               vk::Format::eD32Sfloat,
                                               vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                               vk::ImageAspectFlagBits::eDepth);

    ASSERT_TRUE(handle.isPresent());
    EXPECT_NE(handle->getImageView(), vk::raii::ImageView{VK_NULL_HANDLE});
}

TEST_F(RenderTargetFixture, MultipleTargetsAreDistinct) {
    auto handle0 = resources.createRenderTarget(vk::Extent3D{800, 600, 1},
                                                vk::Format::eB8G8R8A8Srgb,
                                                vk::ImageUsageFlagBits::eColorAttachment |
                                                    vk::ImageUsageFlagBits::eTransientAttachment,
                                                vk::ImageAspectFlagBits::eColor);

    auto handle1 = resources.createRenderTarget(vk::Extent3D{800, 600, 1},
                                                vk::Format::eB8G8R8A8Srgb,
                                                vk::ImageUsageFlagBits::eColorAttachment |
                                                    vk::ImageUsageFlagBits::eTransientAttachment,
                                                vk::ImageAspectFlagBits::eColor);

    ASSERT_TRUE(handle0.isPresent());
    ASSERT_TRUE(handle1.isPresent());
    ASSERT_NE(handle0, handle1);
    ASSERT_NE(&*handle0, &*handle1);
}
