#pragma once

#include "commandPool.hpp"
#include "descriptorPool.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "device.hpp"
#include "deviceBuffer.hpp"
#include "frameSync.hpp"
#include "hostBuffer.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "material.hpp"
#include "physicalDevice.hpp"
#include "pipelineManager.hpp"
#include "scene.hpp"
#include "swapchain.hpp"
#include "texture.hpp"
#include "uploadBuffer.hpp"
#include <SDL3/SDL_vulkan.h>
#include <cstdint>
#include <utility>
#include <vector>

namespace Render {
class Render {
  private:
    Vulkan::Instance instance;
    Vulkan::Surface surface;
    Vulkan::PhysicalDevice physicalDevice;
    Vulkan::Device device;
    Vulkan::CommandPool commandPool;
    Vulkan::DescriptorPool descriptorPool;

    std::optional<Vulkan::DescriptorSetLayout> cameraDescriptorSetLayout;
    std::vector<Vulkan::DescriptorSet> cameraDescriptorSets;

    Vulkan::Swapchain swapchain;
    Vulkan::FrameSync syncObjects;

    Vulkan::UploadBuffer stagingBuffer;
    std::optional<Vulkan::DeviceBuffer> vertexBuffer;
    std::optional<Vulkan::DeviceBuffer> indexBuffer;
    std::vector<Vulkan::HostBuffer> uniformBuffers;
    std::vector<Vulkan::Texture> textures;

    size_t vertexOffset;
    size_t indexOffset;

    std::optional<Material> sky;
    std::vector<Material> materials;

    std::optional<Vulkan::PipelineManager> pipelineManager;

    SDL_Window* window;
    uint32_t frameIndex = 0;
    uint32_t maxFramesInFlight;

  public:
    Render(SDL_Window* window, uint32_t maxFramesInFlight);
    Render(const Render&) = delete;
    Render(Render&&) = delete;
    auto operator=(const Render&) -> Render& = delete;
    auto operator=(Render&&) -> Render& = delete;
    ~Render() { device.get().waitIdle(); }
    void uploadScene(const Scene& scene);
    void render(Scene& scene, float deltaTime);

  private:
    static auto getInstanceExtensions() -> std::vector<const char*> {
        uint32_t count = 0;
        const auto* sdlExts = SDL_Vulkan_GetInstanceExtensions(&count);
        std::vector<const char*> extensions(sdlExts, sdlExts + count);
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
        return extensions;
    }

    static auto getWindowSizeInPixels(SDL_Window* window) -> std::pair<int32_t, int32_t> {
        int w = 0;
        int h = 0;
        SDL_GetWindowSizeInPixels(window, &w, &h);

        return {w, h};
    }
};
} // namespace Render