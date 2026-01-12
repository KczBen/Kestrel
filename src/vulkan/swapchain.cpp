#include "swapchain.hpp"
#include "image.hpp"
#include "vulkan/vulkan.hpp"
#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_raii.hpp>

Vulkan::Swapchain::Swapchain(Vulkan::PhysicalDevice& physicalDevice,
                             Vulkan::Device& device,
                             Vulkan::Surface& surface,
                             SDL_Window* window,
                             vk::SampleCountFlagBits msaaSamples)
    : device(&device.get()), physicalDevice(physicalDevice), swapchainFormat(vk::Format::eB8G8R8A8Srgb),
      msaaSamples(msaaSamples) {
    createSwapchain(device, surface, window);
}

auto Vulkan::Swapchain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    -> vk::SurfaceFormatKHR {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

auto Vulkan::Swapchain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
    -> vk::PresentModeKHR {
    for (const auto& presentMode : availablePresentModes) {
        if (presentMode == vk::PresentModeKHR::eMailbox) {
            return presentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

auto Vulkan::Swapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window)
    -> vk::Extent2D {
    if (capabilities.currentExtent.width != 0xFFFFFFFF) {
        return capabilities.currentExtent;
    }

    int width = 0;
    int height = 0;

    SDL_GetWindowSizeInPixels(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void Vulkan::Swapchain::createSwapchain(Vulkan::Device& device, Vulkan::Surface& surface, SDL_Window* window) {
    auto swapChainSupport = physicalDevice.getSwapChainSupportDetails();
    auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    auto indices = physicalDevice.getQueueFamilyIndices();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    const vk::SwapchainKHR oldSwapchain = *swapchain ? *swapchain : VK_NULL_HANDLE;

    const vk::SwapchainCreateInfoKHR cInfo(
        vk::SwapchainCreateFlagsKHR(),
        surface.get(),
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        indices.graphicsFamily == indices.presentFamily ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
        static_cast<uint32_t>(indices.graphicsFamily == indices.presentFamily ? 0 : 2),
        indices.graphicsFamily == indices.presentFamily ? nullptr : queueFamilyIndices,
        swapChainSupport.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        VK_TRUE,
        oldSwapchain);

    swapchain = vk::raii::SwapchainKHR(*this->device, cInfo);
    swapchainFormat = surfaceFormat.format;

    for (auto img : swapchain.getImages()) {
        images.emplace_back(this->device, img, extent, swapchainFormat, vk::ImageAspectFlagBits::eColor);
    }

    msaaImage = Vulkan::Image(device,
                              vk::Extent3D(extent, 1),
                              swapchainFormat,
                              vk::ImageUsageFlagBits::eColorAttachment,
                              vk::ImageAspectFlagBits::eColor,
                              msaaSamples,
                              1);

    depthImage = Vulkan::Image(device,
                               vk::Extent3D(extent, 1),
                               vk::Format::eD32Sfloat,
                               vk::ImageUsageFlagBits::eDepthStencilAttachment,
                               vk::ImageAspectFlagBits::eDepth,
                               msaaSamples,
                               1);
}

void Vulkan::Swapchain::recreate(Vulkan::Device& device, Vulkan::Surface& surface, SDL_Window* window) {
    int width = 0;
    int height = 0;

    SDL_GetWindowSizeInPixels(window, &width, &height);

    while (width == 0 || height == 0) {
        SDL_GetWindowSizeInPixels(window, &width, &height);
    }

    createSwapchain(device, surface, window);

    this->device->waitIdle();
}

auto Vulkan::Swapchain::get() -> const vk::raii::SwapchainKHR& { return swapchain; }

auto Vulkan::Swapchain::getFormat() -> const vk::Format& { return swapchainFormat; }

auto Vulkan::Swapchain::getExtent() -> const vk::Extent2D& { return extent; }

auto Vulkan::Swapchain::getImages() -> std::span<Vulkan::Image> { return images; }

auto Vulkan::Swapchain::getImage(size_t index) -> Vulkan::Image& { return images[index]; }

auto Vulkan::Swapchain::getDepthImage() -> Vulkan::Image& { return depthImage; }

auto Vulkan::Swapchain::getMsaaImage() -> Vulkan::Image& { return msaaImage; }

auto Vulkan::Swapchain::getMsaaSampleCount() -> vk::SampleCountFlagBits { return msaaSamples; }