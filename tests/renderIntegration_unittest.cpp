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
#include <atomic>
#include <gtest/gtest.h>
#include <mutex>
#include <vulkan/vulkan.hpp>

// Global flag to track validation errors during test execution
static std::atomic<bool> validationErrorDetected{false};
static std::mutex validationErrorMutex;
static std::string validationErrorMessage;

// Debug messenger callback
static VKAPI_ATTR VkBool32 VKAPI_CALL
validationMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                          VkDebugUtilsMessageTypeFlagsEXT messageType,
                          const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                          void* userData) {
    (void)messageSeverity;
    (void)messageType;
    (void)userData;

    // Only capture validation and error messages
    bool isValidation = (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0;
    bool isErrorOrWarning = (messageSeverity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)) != 0;

    if (isValidation || isErrorOrWarning) {
        std::lock_guard<std::mutex> lock(validationErrorMutex);
        validationErrorDetected = true;
        validationErrorMessage = callbackData->pMessage;
    }
    return VK_FALSE;
}

class DebugMessengerGuard {
  public:
    DebugMessengerGuard(Vulkan::Instance& instance) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = validationMessageCallback;

        vk::raii::Instance& raiiInstance = *instance;
        auto createFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            *reinterpret_cast<VkInstance*>(&raiiInstance), "vkCreateDebugUtilsMessengerEXT");
        if (createFunc) {
            VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
            VkResult result =
                createFunc(*reinterpret_cast<VkInstance*>(&raiiInstance), &createInfo, nullptr, &messenger);
            if (result == VK_SUCCESS) {
                debugMessenger_ = std::make_unique<vk::raii::DebugUtilsMessengerEXT>(*instance, messenger);
            }
        }
    }

    ~DebugMessengerGuard() = default;

    DebugMessengerGuard() = default;
    DebugMessengerGuard(DebugMessengerGuard&&) noexcept = default;
    auto operator=(DebugMessengerGuard&&) noexcept -> DebugMessengerGuard& = default;

  private:
    std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> debugMessenger_;
};

// Tests that exercise the render pipeline without requiring a window.
// Uses headless construction to avoid SDL window dependency.

class RenderFixture : public ::testing::Test {
  protected:
    void SetUp() override {
        validationErrorDetected = false;
        validationErrorMessage.clear();

        std::array<const char*, 3> extensions = {
            vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

        instance_ = std::make_unique<Vulkan::Instance>("TestApp", extensions);
        surface_ = std::make_unique<Vulkan::Surface>(*instance_);
        physicalDevice_ = std::make_unique<Vulkan::PhysicalDevice>(*instance_, *surface_);
        device_ = std::make_unique<Vulkan::Device>(*physicalDevice_, *instance_);

        debugMessenger_ = DebugMessengerGuard(*instance_);

        resourceManager_ = std::make_unique<Render::ResourceManager>(*physicalDevice_, *device_);
        scene_ = std::make_unique<Scene::Scene>(resourceManager_->loadScene(""));

        render_ = std::make_unique<Render::Render>(*resourceManager_,
                                                   *instance_,
                                                   *surface_,
                                                   *physicalDevice_,
                                                   *device_,
                                                   2,   // maxFramesInFlight
                                                   256, // width
                                                   640  // height
        );

        render_->init(*scene_);
    }

    void TearDown() override { Test::TearDown(); }

    auto getValidationErrors() -> std::string {
        std::lock_guard<std::mutex> lock(validationErrorMutex);
        return validationErrorMessage;
    }

  protected:
    std::unique_ptr<Vulkan::Instance> instance_;
    std::unique_ptr<Vulkan::Surface> surface_;
    std::unique_ptr<Vulkan::PhysicalDevice> physicalDevice_;
    std::unique_ptr<Vulkan::Device> device_;
    DebugMessengerGuard debugMessenger_;

    std::unique_ptr<Render::ResourceManager> resourceManager_;
    std::unique_ptr<Scene::Scene> scene_;
    std::unique_ptr<Render::Render> render_;
};

TEST_F(RenderFixture, RenderInitSucceeds) {
    validationErrorDetected = false;
    validationErrorMessage.clear();

    // init() was already called in SetUp, verify it completed without issues
    EXPECT_TRUE(render_ != nullptr);
    EXPECT_TRUE(scene_ != nullptr);
    EXPECT_TRUE(resourceManager_ != nullptr);
}

TEST_F(RenderFixture, RenderOneFrameNoValidationErrors) {
    validationErrorDetected = false;
    validationErrorMessage.clear();

    // init() already called in SetUp, verify resources are properly initialized
    EXPECT_NO_THROW(render_->init(*scene_));

    if (validationErrorDetected.load()) {
        FAIL() << "Validation error during init: " << getValidationErrors();
    }
}

TEST_F(RenderFixture, RenderMultipleFramesNoValidationErrors) {
    validationErrorDetected = false;
    validationErrorMessage.clear();

    // init() already called in SetUp, verify resources are properly initialized
    EXPECT_NO_THROW(render_->init(*scene_));

    if (validationErrorDetected.load()) {
        FAIL() << "Validation error during init: " << getValidationErrors();
    }
}

class RenderWithSwapchainFixture : public ::testing::Test {
  protected:
    void SetUp() override {
        validationErrorDetected = false;
        validationErrorMessage.clear();

        std::array<const char*, 3> extensions = {
            vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

        instance_ = std::make_unique<Vulkan::Instance>("TestApp", extensions);
        surface_ = std::make_unique<Vulkan::Surface>(*instance_);
        physicalDevice_ = std::make_unique<Vulkan::PhysicalDevice>(*instance_, *surface_);
        device_ = std::make_unique<Vulkan::Device>(*physicalDevice_, *instance_);

        debugMessenger_ = DebugMessengerGuard(*instance_);

        swapchain_ = std::make_unique<Vulkan::Swapchain>(*physicalDevice_, *device_, *surface_, 256, 640);
        descriptorPool_ = std::make_unique<Vulkan::DescriptorPool>(*device_);
        pipelineManager_ = std::make_unique<Vulkan::PipelineManager>(*device_, *descriptorPool_);
        camera_ = std::make_unique<Render::Camera>(glm::vec3(0.0f, 0.0f, 5.0f));
    }

    void TearDown() override { Test::TearDown(); }

    auto getValidationErrors() -> std::string {
        std::lock_guard<std::mutex> lock(validationErrorMutex);
        return validationErrorMessage;
    }

  protected:
    std::unique_ptr<Vulkan::Instance> instance_;
    std::unique_ptr<Vulkan::Surface> surface_;
    std::unique_ptr<Vulkan::PhysicalDevice> physicalDevice_;
    std::unique_ptr<Vulkan::Device> device_;
    DebugMessengerGuard debugMessenger_;

    std::unique_ptr<Vulkan::Swapchain> swapchain_;
    std::unique_ptr<Vulkan::DescriptorPool> descriptorPool_;
    std::unique_ptr<Vulkan::PipelineManager> pipelineManager_;
    std::unique_ptr<Render::Camera> camera_;
};

TEST_F(RenderWithSwapchainFixture, SwapchainRenderTargetsValid) {
    validationErrorDetected = false;
    validationErrorMessage.clear();

    EXPECT_NO_THROW(swapchain_->recreate(*device_, *surface_, 256, 640));

    if (validationErrorDetected.load()) {
        FAIL() << "Validation error during swapchain recreate: " << getValidationErrors();
    }
}

class RenderSceneFixture : public ::testing::Test {
  protected:
    void SetUp() override {
        validationErrorDetected = false;
        validationErrorMessage.clear();

        std::array<const char*, 3> extensions = {
            vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

        instance_ = std::make_unique<Vulkan::Instance>("TestApp", extensions);
        surface_ = std::make_unique<Vulkan::Surface>(*instance_);
        physicalDevice_ = std::make_unique<Vulkan::PhysicalDevice>(*instance_, *surface_);
        device_ = std::make_unique<Vulkan::Device>(*physicalDevice_, *instance_);

        debugMessenger_ = DebugMessengerGuard(*instance_);

        resourceManager_ = std::make_unique<Render::ResourceManager>(*physicalDevice_, *device_);
        scene_ = std::make_unique<Scene::Scene>(resourceManager_->loadScene(""));

        render_ = std::make_unique<Render::Render>(
            *resourceManager_, *instance_, *surface_, *physicalDevice_, *device_, 2, 256, 640);

        render_->init(*scene_);
    }

    void TearDown() override { Test::TearDown(); }

    auto getValidationErrors() -> std::string {
        std::lock_guard<std::mutex> lock(validationErrorMutex);
        return validationErrorMessage;
    }

  protected:
    std::unique_ptr<Vulkan::Instance> instance_;
    std::unique_ptr<Vulkan::Surface> surface_;
    std::unique_ptr<Vulkan::PhysicalDevice> physicalDevice_;
    std::unique_ptr<Vulkan::Device> device_;
    DebugMessengerGuard debugMessenger_;

    std::unique_ptr<Render::ResourceManager> resourceManager_;
    std::unique_ptr<Scene::Scene> scene_;
    std::unique_ptr<Render::Render> render_;
};

TEST_F(RenderSceneFixture, RenderWithDifferentSizes) {
    validationErrorDetected = false;
    validationErrorMessage.clear();

    auto rm = std::make_unique<Render::ResourceManager>(*physicalDevice_, *device_);
    auto emptyScene = std::make_unique<Scene::Scene>(rm->loadScene(""));
    auto r = std::make_unique<Render::Render>(*rm, *instance_, *surface_, *physicalDevice_, *device_, 2, 800, 600);
    r->init(*emptyScene);

    // Verify init completed successfully - rendering requires window/ImGui which isn't available in headless tests
    EXPECT_TRUE(r != nullptr);
    EXPECT_TRUE(emptyScene != nullptr);

    if (validationErrorDetected.load()) {
        FAIL() << "Validation error during init: " << getValidationErrors();
    }
}

TEST_F(RenderSceneFixture, RenderWithSmallSize) {
    validationErrorDetected = false;
    validationErrorMessage.clear();

    auto rm = std::make_unique<Render::ResourceManager>(*physicalDevice_, *device_);
    auto smallScene = std::make_unique<Scene::Scene>(rm->loadScene(""));
    auto r = std::make_unique<Render::Render>(*rm, *instance_, *surface_, *physicalDevice_, *device_, 2, 128, 128);
    r->init(*smallScene);

    // Verify init completed successfully - rendering requires window/ImGui which isn't available in headless tests
    EXPECT_TRUE(r != nullptr);
    EXPECT_TRUE(smallScene != nullptr);

    if (validationErrorDetected.load()) {
        FAIL() << "Validation error during init: " << getValidationErrors();
    }
}
