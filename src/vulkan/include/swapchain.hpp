#pragma once

#include "device.hpp"
#include "image.hpp"
#include "physicalDevice.hpp"
#include "vulkan/vulkan.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class Swapchain {
  private:
    const vk::raii::Device* device;
    Vulkan::PhysicalDevice physicalDevice;
    vk::raii::SwapchainKHR swapchain = nullptr;
    std::vector<Vulkan::Image> images;
    Vulkan::Image depthImage;
    Vulkan::Image msaaImage;
    vk::Format swapchainFormat;
    vk::Extent2D extent;

    vk::SampleCountFlagBits msaaSamples;

  public:
    Swapchain(Vulkan::PhysicalDevice& physicalDevice,
              Vulkan::Device& device,
              Vulkan::Surface& surface,
              SDL_Window* window,
              vk::SampleCountFlagBits msaaSamples);
    void recreate(Vulkan::Device& device, Vulkan::Surface& surface, SDL_Window* window);
    auto get() -> const vk::raii::SwapchainKHR&;
    auto getFormat() -> const vk::Format&;
    auto getExtent() -> const vk::Extent2D&;
    auto getImages() -> std::span<Vulkan::Image>;
    auto getImage(size_t index) -> Vulkan::Image&;
    auto getDepthImage() -> Vulkan::Image&;
    auto getMsaaImage() -> Vulkan::Image&;
    auto getMsaaSampleCount() -> vk::SampleCountFlagBits;

  private:
    void createSwapchain(Vulkan::Device& device, Vulkan::Surface& surface, SDL_Window* window);
    static auto chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
        -> vk::SurfaceFormatKHR;
    static auto chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
        -> vk::PresentModeKHR;
    static auto chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window) -> vk::Extent2D;
    void createImageViews();
};
} // namespace Vulkan