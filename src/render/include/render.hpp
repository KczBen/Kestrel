#pragma once

#include "commandPool.hpp"
#include "descriptorPool.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "device.hpp"
#include "deviceBuffer.hpp"
#include "frameSync.hpp"
#include "graphicsPipeline.hpp"
#include "hostBuffer.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "scene.hpp"
#include "swapchain.hpp"
#include "texture.hpp"
#include "uploadBuffer.hpp"
#include <SDL3/SDL_video.h>
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

    std::optional<Vulkan::DescriptorSetLayout> materialDescriptorSetLayout;
    std::vector<Vulkan::DescriptorSet> materialDescriptorSets;

    Vulkan::Swapchain swapchain;
    Vulkan::FrameSync syncObjects;

    Vulkan::UploadBuffer stagingBuffer;
    std::optional<Vulkan::DeviceBuffer> vertexBuffer;
    std::optional<Vulkan::DeviceBuffer> indexBuffer;
    std::vector<Vulkan::HostBuffer> uniformBuffers;
    std::vector<Vulkan::Texture> textures;

    std::optional<Vulkan::GraphicsPipeline> pipeline;

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
};
} // namespace Render