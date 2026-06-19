#include "swapchain.hpp"
#include "image.hpp"
#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_raii.hpp>

Vulkan::Swapchain::Swapchain(Vulkan::PhysicalDevice& physicalDevice,
                             Vulkan::Device& device,
                             Vulkan::Surface& surface,
                             uint32_t width,
                             uint32_t height)
    : device(&device.get()), physicalDevice(physicalDevice), swapchainFormat(vk::Format::eB8G8R8A8Srgb) {
    createSwapchain(device, surface, width, height);
}

Vulkan::Swapchain::Swapchain(Vulkan::PhysicalDevice& physicalDevice,
                             Vulkan::Device& device,
                             Vulkan::Surface& surface,
                             std::pair<uint32_t, uint32_t> extent)
    : device(&device.get()), physicalDevice(physicalDevice), swapchainFormat(vk::Format::eB8G8R8A8Srgb) {
    createSwapchain(device, surface, extent.first, extent.second);
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

auto Vulkan::Swapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
                                         uint32_t width,
                                         uint32_t height) -> vk::Extent2D {
    if (capabilities.currentExtent.width != 0xFFFFFFFF) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = {width, height};
    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void Vulkan::Swapchain::createSwapchain(Vulkan::Device& device,
                                        Vulkan::Surface& surface,
                                        uint32_t width,
                                        uint32_t height) {
    auto swapChainSupport = physicalDevice.getSwapChainSupportDetails();
    auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    extent = chooseSwapExtent(swapChainSupport.capabilities, width, height);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    auto indices = physicalDevice.getQueueFamilyIndices();
    if (!indices.graphicsFamily.has_value() || !indices.presentFamily.has_value()) {
        throw std::runtime_error("Required queue family not found");
    }
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    // SwapchainCreateInfoKHR needs the raw handle, not RAII
    const vk::SwapchainKHR oldSwapchain = *swapchain ? *swapchain : VK_NULL_HANDLE;

    const vk::SwapchainCreateInfoKHR cInfo(
        vk::SwapchainCreateFlagsKHR(),
        surface.get(),
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        indices.graphicsFamily == indices.presentFamily ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
        static_cast<uint32_t>(indices.graphicsFamily == indices.presentFamily ? 0 : 2),
        indices.graphicsFamily == indices.presentFamily ? nullptr : queueFamilyIndices,
        swapChainSupport.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        vk::True,
        oldSwapchain);

    swapchain = vk::raii::SwapchainKHR(*this->device, cInfo);
    swapchainFormat = surfaceFormat.format;

    for (auto img : swapchain.getImages()) {
        images.emplace_back(this->device, img, extent, swapchainFormat, vk::ImageAspectFlagBits::eColor);
    }
}

void Vulkan::Swapchain::recreate(Vulkan::Device& device, Vulkan::Surface& surface, uint32_t width, uint32_t height) {
    createSwapchain(device, surface, width, height);

    this->device->waitIdle();
}

auto Vulkan::Swapchain::get() -> const vk::raii::SwapchainKHR& { return swapchain; }

auto Vulkan::Swapchain::getFormat() -> const vk::Format& { return swapchainFormat; }

auto Vulkan::Swapchain::getExtent() -> const vk::Extent2D& { return extent; }

auto Vulkan::Swapchain::getImages() -> std::span<Vulkan::Image> { return images; }

auto Vulkan::Swapchain::getImage(size_t index) -> Vulkan::Image& { return images[index]; }