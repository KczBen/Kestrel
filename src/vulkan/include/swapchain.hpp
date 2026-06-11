#pragma once

#include "device.hpp"
#include "image.hpp"
#include "physicalDevice.hpp"
#include "renderTarget.hpp"
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

    std::vector<Render::RenderTarget> renderTargets;

  public:
    Swapchain(Vulkan::PhysicalDevice& physicalDevice,
              Vulkan::Device& device,
              Vulkan::Surface& surface,
              uint32_t width,
              uint32_t height,
              vk::SampleCountFlagBits msaaSamples);

    Swapchain(Vulkan::PhysicalDevice& physicalDevice,
              Vulkan::Device& device,
              Vulkan::Surface& surface,
              std::pair<uint32_t, uint32_t> extent,
              vk::SampleCountFlagBits msaaSamples);

    void recreate(Vulkan::Device& device, Vulkan::Surface& surface, uint32_t width, uint32_t height);
    auto get() -> const vk::raii::SwapchainKHR&;
    auto getFormat() -> const vk::Format&;
    auto getExtent() -> const vk::Extent2D&;
    auto getImages() -> std::span<Vulkan::Image>;
    auto getImage(size_t index) -> Vulkan::Image&;
    auto getDepthImage() -> Vulkan::Image&;
    auto getMsaaImage() -> Vulkan::Image&;
    auto getMsaaSampleCount() -> vk::SampleCountFlagBits;
    auto getRenderTargets() -> std::span<Render::RenderTarget>;
    auto getRenderTarget(uint32_t index) -> Render::RenderTarget&;

  private:
    void createSwapchain(Vulkan::Device& device, Vulkan::Surface& surface, uint32_t width, uint32_t height);
    static auto chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
        -> vk::SurfaceFormatKHR;
    static auto chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
        -> vk::PresentModeKHR;
    static auto chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
        -> vk::Extent2D;
    void createImageViews();
};
} // namespace Vulkan