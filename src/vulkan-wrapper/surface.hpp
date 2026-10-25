#pragma once

#include "instance.hpp"
#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class Surface {
  private:
    vk::raii::SurfaceKHR surface = nullptr;

  public:
    Surface(SDL_Window* window, Vulkan::Instance& instance);

    // Only for testing! Makes a headless surface, it's useless for other tasks
    Surface(Vulkan::Instance& instance);
    [[nodiscard]] auto operator*() -> vk::raii::SurfaceKHR& { return surface; }
    [[nodiscard]] auto operator->() -> vk::raii::SurfaceKHR* { return &surface; }
    auto get() -> vk::raii::SurfaceKHR&;
};
} // namespace Vulkan