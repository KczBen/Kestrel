#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_video.h>
#include <gtest/gtest.h>

using namespace Vulkan;

TEST(PhysicalDeviceTest, GettersReturnNonNull) {
    ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window* window = SDL_CreateWindow("TestWindow", 640, 480, SDL_WINDOW_VULKAN);
    ASSERT_TRUE(window != nullptr);
    Instance instance("TestApp", window);
    Surface surface(window, instance);
    PhysicalDevice physicalDevice(instance, surface);

    ASSERT_TRUE(physicalDevice.get() != nullptr);
    auto ext = physicalDevice.getExtensions();
    ASSERT_FALSE(ext.empty());
    auto indices = physicalDevice.getQueueFamilyIndices();
    ASSERT_TRUE(indices.graphicsFamily.has_value());
    ASSERT_TRUE(indices.presentFamily.has_value());
    ASSERT_TRUE(indices.computeFamily.has_value());
    auto swapSupport = physicalDevice.getSwapChainSupportDetails();
    // Technically not a fault in our code, all compliant devices with a swapchain should support *a* format
    ASSERT_FALSE(swapSupport.formats.empty());
    ASSERT_FALSE(swapSupport.presentModes.empty());

    SDL_DestroyWindow(window);
    SDL_Quit();
}
