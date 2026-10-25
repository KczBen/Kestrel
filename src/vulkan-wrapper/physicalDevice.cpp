#include <set>
#include <vulkan/vulkan_core.h>

#include "physicalDevice.hpp"
#include "surface.hpp"

Vulkan::PhysicalDevice::PhysicalDevice(Vulkan::Instance& instance, Vulkan::Surface& surface) {
    auto devices = (*instance).enumeratePhysicalDevices();

    for (auto& device : devices) {
        if (isDeviceSuitable(surface, device)) {
            physicalDevice = device;
            break;
        }
    }
}

auto Vulkan::PhysicalDevice::isDeviceSuitable(Vulkan::Surface& surface, vk::raii::PhysicalDevice& device) -> bool {
    findQueueFamilies(surface, device);
    const bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        swapChainSupportdetails = querySwapChainSupport(surface, device);
        swapChainAdequate = !swapChainSupportdetails.formats.empty() && !swapChainSupportdetails.presentModes.empty();
    }

    return queueFamilyIndices.isComplete() && extensionsSupported && swapChainAdequate;
}

void Vulkan::PhysicalDevice::findQueueFamilies(Vulkan::Surface& surface, vk::raii::PhysicalDevice& device) {
    auto queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            queueFamilyIndices.graphicsFamily = i;
        }

        if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) {
            queueFamilyIndices.computeFamily = i;
        }

        auto presentSupport = device.getSurfaceSupportKHR(i, *surface);

        if (presentSupport != 0u) {
            queueFamilyIndices.presentFamily = i;
        }

        if (queueFamilyIndices.isComplete()) {
            break;
        }

        i++;
    }
}

auto Vulkan::PhysicalDevice::checkDeviceExtensionSupport(vk::raii::PhysicalDevice& device) -> bool {
    auto availableExtensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

auto Vulkan::PhysicalDevice::querySwapChainSupport(Vulkan::Surface& surface, vk::raii::PhysicalDevice& device)
    -> Vulkan::PhysicalDevice::SwapChainSupportDetails {
    SwapChainSupportDetails details{.capabilities = device.getSurfaceCapabilitiesKHR(*surface),
                                    .formats = device.getSurfaceFormatsKHR(*surface),
                                    .presentModes = device.getSurfacePresentModesKHR(*surface)};

    return details;
}

auto Vulkan::PhysicalDevice::get() -> vk::raii::PhysicalDevice& { return physicalDevice; }

auto Vulkan::PhysicalDevice::getExtensions() -> const std::vector<const char*>& { return deviceExtensions; }

auto Vulkan::PhysicalDevice::getQueueFamilyIndices() -> const Vulkan::PhysicalDevice::QueueFamilyIndices& {
    return queueFamilyIndices;
}

auto Vulkan::PhysicalDevice::getSwapChainSupportDetails() -> const Vulkan::PhysicalDevice::SwapChainSupportDetails& {
    return swapChainSupportdetails;
}