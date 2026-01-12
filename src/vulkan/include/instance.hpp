#pragma once

#include "vulkan/vulkan_raii.hpp"
#include <SDL3/SDL_video.h>
#include <string>

namespace Vulkan {
class Instance {
  private:
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;

  public:
    Instance(const std::string& appName, SDL_Window* window);
    auto get() -> vk::raii::Instance&;
};
} // namespace Vulkan