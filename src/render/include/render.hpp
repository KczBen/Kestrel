#pragma once

#include "commandPool.hpp"
#include "device.hpp"
#include "frameSync.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "pass.hpp"
#include "physicalDevice.hpp"
#include "resourceManager.hpp"
#include "scene.hpp"
#include "swapchain.hpp"
#include <SDL3/SDL_vulkan.h>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace Render {
class Render {
  private:
    Vulkan::Instance* instance;
    Vulkan::Surface* surface;
    Vulkan::PhysicalDevice* physicalDevice;
    Vulkan::Device* device;
    ResourceManager* resourceManager;
    Vulkan::CommandPool commandPool;

    Vulkan::Swapchain swapchain;
    Vulkan::FrameSync syncObjects;

    std::vector<std::unique_ptr<Pass>> passes;

    SDL_Window* window;
    uint32_t frameIndex = 0;
    uint32_t maxFramesInFlight;

  public:
    Render(SDL_Window* window,
           ResourceManager& resources,
           Vulkan::Instance& instance,
           Vulkan::Surface& surface,
           Vulkan::PhysicalDevice& physicalDevice,
           Vulkan::Device& device,
           uint32_t maxFramesInFlight);
    Render(const Render&) = delete;
    Render(Render&&) = delete;
    auto operator=(const Render&) -> Render& = delete;
    auto operator=(Render&&) -> Render& = delete;
    ~Render() { device->get().waitIdle(); }
    void init(Scene& scene);
    void render(Scene& scene, float deltaTime);

  private:
    static auto getWindowSizeInPixels(SDL_Window* window) -> std::pair<int32_t, int32_t> {
        int w = 0;
        int h = 0;
        SDL_GetWindowSizeInPixels(window, &w, &h);

        return {w, h};
    }
};
} // namespace Render