#include "camera.hpp"
#include "commandPool.hpp"
#include "device.hpp"
#include "frameSync.hpp"
#include "instance.hpp"
#include "material.hpp"
#include "pass.hpp"
#include "physicalDevice.hpp"
#include "pipelineManager.hpp"
#include "render.hpp"
#include "renderTarget.hpp"
#include "resourceManager.hpp"
#include "scene.hpp"
#include "surface.hpp"
#include "swapchain.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

using namespace Vulkan;

// Tests that exercise the render pipeline without requiring a window.
// Uses headless construction to avoid SDL window dependency.

class RenderFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};

    Render::ResourceManager resourceManager{physicalDevice, device};

    Render::Scene scene{std::vector<std::string>{}};

    Render::Render render{
        resourceManager,
        instance,
        surface,
        physicalDevice,
        device,
        2,   // maxFramesInFlight
        256, // width
        640  // height
    };

    void SetUp() override {
        resourceManager.loadScene(scene);
        render.init(scene);
    }
};

TEST_F(RenderFixture, RenderInitSucceeds) { EXPECT_NO_THROW(render.init(scene)); }

TEST_F(RenderFixture, RenderOneFrameNoValidationErrors) { EXPECT_NO_THROW(render.render(scene, 0.016f)); }

TEST_F(RenderFixture, RenderMultipleFramesNoValidationErrors) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(render.render(scene, 0.016f));
    }
}

class RenderWithSwapchainFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};

    Swapchain swapchain{physicalDevice, device, surface, 256, 640};
    DescriptorPool descriptorPool{device};

    PipelineManager pipelineManager{device, descriptorPool};

    Render::Camera camera{glm::vec3(0.0f, 0.0f, 5.0f)};
};

TEST_F(RenderWithSwapchainFixture, SwapchainRenderTargetsValid) {
    // Render targets are now managed by ResourceManager, not Swapchain
    // This test verifies the swapchain can still be created without issues
    EXPECT_NO_THROW(swapchain.recreate(device, surface, 256, 640));
}

class RenderSceneFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};

    Render::ResourceManager resourceManager{physicalDevice, device};
    Render::Scene scene{std::vector<std::string>{}};

    Render::Render render{resourceManager, instance, surface, physicalDevice, device, 2, 256, 640};

    void SetUp() override {
        resourceManager.loadScene(scene);
        render.init(scene);
    }
};

TEST_F(RenderSceneFixture, RenderWithDifferentSizes) {
    Render::ResourceManager rm{physicalDevice, device};
    Render::Scene emptyScene{std::vector<std::string>{}};
    Render::Render r{rm, instance, surface, physicalDevice, device, 2, 800, 600};
    rm.loadScene(emptyScene);
    r.init(emptyScene);
    EXPECT_NO_THROW(r.render(emptyScene, 0.016f));
}

TEST_F(RenderSceneFixture, RenderWithSmallSize) {
    Render::ResourceManager rm{physicalDevice, device};
    Render::Scene smallScene{std::vector<std::string>{}};
    Render::Render r{rm, instance, surface, physicalDevice, device, 2, 128, 128};
    rm.loadScene(smallScene);
    r.init(smallScene);
    EXPECT_NO_THROW(r.render(smallScene, 0.016f));
}
