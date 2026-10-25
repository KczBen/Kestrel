#pragma once

#include "frameGraph.hpp"
#include "resourceManager.hpp"
#include "scene/scene.hpp"
#include "vulkan-wrapper/commandPool.hpp"
#include "vulkan-wrapper/device.hpp"
#include "vulkan-wrapper/frameSync.hpp"
#include "vulkan-wrapper/image.hpp"
#include "vulkan-wrapper/instance.hpp"
#include "vulkan-wrapper/physicalDevice.hpp"
#include "vulkan-wrapper/swapchain.hpp"

#include <SDL3/SDL_vulkan.h>
#include <cstdint>
#include <utility>

namespace Render {
class Render {
  private:
    Vulkan::Instance* instance;
    Vulkan::Surface* surface;
    Vulkan::PhysicalDevice* physicalDevice;
    Vulkan::Device* device;
    ResourceManager* resources;
    Vulkan::CommandPool commandPool;

    Vulkan::Swapchain swapchain;
    Vulkan::FrameSync syncObjects;

    FrameGraph frameGraph;

    RenderTargetHandle colorTarget;
    RenderTargetHandle depthTarget;

    SDL_Window* window;
    uint32_t frameIndex = 0;
    uint32_t maxFramesInFlight;
    uint32_t width;
    uint32_t height;

  public:
    Render(SDL_Window* window,
           ResourceManager& resources,
           Vulkan::Instance& instance,
           Vulkan::Surface& surface,
           Vulkan::PhysicalDevice& physicalDevice,
           Vulkan::Device& device,
           uint32_t maxFramesInFlight);

    Render(ResourceManager& resources,
           Vulkan::Instance& instance,
           Vulkan::Surface& surface,
           Vulkan::PhysicalDevice& physicalDevice,
           Vulkan::Device& device,
           uint32_t maxFramesInFlight,
           uint32_t width,
           uint32_t height);

    Render(const Render&) = delete;
    Render(Render&&) = delete;
    auto operator=(const Render&) -> Render& = delete;
    auto operator=(Render&&) -> Render& = delete;
    ~Render() { device->get().waitIdle(); }
    void init(Scene::Scene& scene);
    void render(Scene::Scene& scene, float deltaTime);

  private:
    static auto getWindowSizeInPixels(SDL_Window* window) -> std::pair<uint32_t, uint32_t> {
        int w = 0;
        int h = 0;
        SDL_GetWindowSizeInPixels(window, &w, &h);

        return {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    }

    auto getCurrentExtent() -> std::pair<uint32_t, uint32_t> {
        if (window != nullptr) {
            return getWindowSizeInPixels(window);
        }
        return {width, height};
    }
};
} // namespace Render