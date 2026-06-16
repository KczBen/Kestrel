#include "commandPool.hpp"
#include "device.hpp"
#include "frameSync.hpp"
#include "instance.hpp"
#include "material.hpp"
#include "physicalDevice.hpp"
#include "render.hpp"
#include "renderTarget.hpp"
#include "resourceManager.hpp"
#include "scene.hpp"
#include "surface.hpp"
#include "swapchain.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Vulkan;

class HeadlessRenderFixture : public ::testing::Test {
  protected:
    void SetUp() override {
        instance = std::make_unique<Instance>(
            "HeadlessTest",
            std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName});
        surface = std::make_unique<Surface>(*instance);
        physicalDevice = std::make_unique<PhysicalDevice>(*instance, *surface);
        device = std::make_unique<Device>(*physicalDevice, *instance);
        resourceManager = std::make_unique<Render::ResourceManager>(*physicalDevice, *device);
        scene = std::make_unique<Render::Scene>(std::vector<std::string>{});
        renderer = std::make_unique<Render::Render>(
            800, 600, *resourceManager, *instance, *surface, *physicalDevice, *device, 2);
        resourceManager->loadScene(*scene);
    }

    std::unique_ptr<Instance> instance;
    std::unique_ptr<Surface> surface;
    std::unique_ptr<PhysicalDevice> physicalDevice;
    std::unique_ptr<Device> device;
    std::unique_ptr<Render::ResourceManager> resourceManager;
    std::unique_ptr<Render::Scene> scene;
    std::unique_ptr<Render::Render> renderer;
};

TEST_F(HeadlessRenderFixture, CreateRenderHeadless) {
    renderer->init(*scene);
    renderer->render(*scene, 0.016f);
}

TEST_F(HeadlessRenderFixture, SingleFrameDoesNotThrow) {
    renderer->init(*scene);
    EXPECT_NO_THROW(renderer->render(*scene, 0.016f));
}

TEST_F(HeadlessRenderFixture, MultipleFramesDoNotThrow) {
    renderer->init(*scene);

    EXPECT_NO_THROW(renderer->render(*scene, 0.016f));
    EXPECT_NO_THROW(renderer->render(*scene, 0.016f));
    EXPECT_NO_THROW(renderer->render(*scene, 0.016f));
}

TEST_F(HeadlessRenderFixture, SmallResolution) {
    renderer =
        std::make_unique<Render::Render>(320, 240, *resourceManager, *instance, *surface, *physicalDevice, *device, 2);
    resourceManager->loadScene(*scene);
    renderer->init(*scene);
    EXPECT_NO_THROW(renderer->render(*scene, 0.016f));
}

TEST_F(HeadlessRenderFixture, LargeResolution) {
    renderer = std::make_unique<Render::Render>(
        1920, 1080, *resourceManager, *instance, *surface, *physicalDevice, *device, 2);
    resourceManager->loadScene(*scene);
    renderer->init(*scene);
    EXPECT_NO_THROW(renderer->render(*scene, 0.016f));
}
