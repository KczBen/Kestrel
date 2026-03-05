#include "device.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_video.h>
#include <gtest/gtest.h>

using namespace Vulkan;

TEST(DeviceTest, GettersReturnNonNull) {
    ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window* window = SDL_CreateWindow("TestWindow", 640, 480, SDL_WINDOW_VULKAN);
    ASSERT_NE(window, nullptr);

    Instance instance("TestApp", window);
    Surface surface(window, instance);

    PhysicalDevice physicalDevice(instance, surface);

    Device device(physicalDevice, instance);

    ASSERT_NE(device.get(), nullptr);
    ASSERT_NE(device.getPhysicalDevice(), nullptr);
    ASSERT_NE(device.getGraphicsQueue(), nullptr);
    ASSERT_NE(device.getPresentQueue(), nullptr);
    ASSERT_NE(device.getAllocator(), nullptr);

    // Clean up
    SDL_DestroyWindow(window);
    SDL_Quit();
}
