#include "surface.hpp"
#include "SDL3/SDL_vulkan.h"
#include <vulkan/vulkan_raii.hpp>

Vulkan::Surface::Surface(SDL_Window* window, Vulkan::Instance& instance) {
    VkSurfaceKHR rawSurface{};
    SDL_Vulkan_CreateSurface(window, *instance.get(), nullptr, &rawSurface);

    surface = vk::raii::SurfaceKHR(instance.get(), rawSurface);
}

auto Vulkan::Surface::get() -> vk::raii::SurfaceKHR& { return surface; }