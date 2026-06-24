#pragma once

#include "instance.hpp"
#include "surface.hpp"
#include "vulkan/vulkan.hpp"
#include <cstdint>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class PhysicalDevice {
  private:
    vk::raii::PhysicalDevice physicalDevice = nullptr;

    std::vector<const char*> deviceExtensions = {vk::KHRSwapchainExtensionName,
                                                 vk::KHRSpirv14ExtensionName,
                                                 vk::KHRSynchronization2ExtensionName,
                                                 vk::KHRCreateRenderpass2ExtensionName};

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> computeFamily;

        [[nodiscard]] auto isComplete() const -> bool {
            return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
        }
    };

    QueueFamilyIndices queueFamilyIndices;

    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    SwapChainSupportDetails swapChainSupportdetails;

    auto isDeviceSuitable(Vulkan::Surface& surface, vk::raii::PhysicalDevice& device) -> bool;
    void findQueueFamilies(Vulkan::Surface& surface, vk::raii::PhysicalDevice& device);
    auto checkDeviceExtensionSupport(vk::raii::PhysicalDevice& device) -> bool;
    static auto querySwapChainSupport(Vulkan::Surface& surface, vk::raii::PhysicalDevice& device)
        -> SwapChainSupportDetails;

  public:
    PhysicalDevice(Vulkan::Instance& instance, Vulkan::Surface& surface);
    auto getExtensions() -> const std::vector<const char*>&;
    auto getQueueFamilyIndices() -> const QueueFamilyIndices&;
    auto getSwapChainSupportDetails() -> const SwapChainSupportDetails&;
    [[nodiscard]] auto operator*() -> vk::raii::PhysicalDevice& { return physicalDevice; }
    [[nodiscard]] auto operator->() -> vk::raii::PhysicalDevice* { return &physicalDevice; }
    auto get() -> vk::raii::PhysicalDevice&;
};
} // namespace Vulkan