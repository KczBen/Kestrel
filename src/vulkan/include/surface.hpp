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
    auto get() -> vk::raii::SurfaceKHR&;
};
} // namespace Vulkan