#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>
#define VMA_IMPLEMENTATION

#include <set>

#include "device.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"

Vulkan::Device::Device(Vulkan::PhysicalDevice& physicalDevice, Vulkan::Instance& instance)
    : physicalDevice(&*physicalDevice), memoryAllocator(nullptr) {
    auto indices = physicalDevice.getQueueFamilyIndices();
    const float queuePriority = 1.0F;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    const std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    for (const uint32_t queueFamily : uniqueQueueFamilies) {
        const vk::DeviceQueueCreateInfo queueCreateInfo(vk::DeviceQueueCreateFlags(), queueFamily, 1, &queuePriority);

        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan12Features,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain;

    auto& deviceFeatures = featureChain.get<vk::PhysicalDeviceFeatures2>();
    deviceFeatures.features.samplerAnisotropy = vk::True;

    auto& vulkan12 = featureChain.get<vk::PhysicalDeviceVulkan12Features>();
    vulkan12.shaderFloat16 = vk::True;

    auto& vulkan13 = featureChain.get<vk::PhysicalDeviceVulkan13Features>();
    vulkan13.dynamicRendering = vk::True;
    vulkan13.synchronization2 = vk::True;

    auto& extDynamic = featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    extDynamic.extendedDynamicState = vk::True;

    const vk::DeviceCreateInfo deviceCreateInfo(vk::DeviceCreateFlags(),
                                                queueCreateInfos,
                                                {},
                                                physicalDevice.getExtensions(),
                                                nullptr,
                                                &featureChain.get<vk::PhysicalDeviceFeatures2>());

    logicalDevice = vk::raii::Device(*physicalDevice, deviceCreateInfo);

    graphicsQueue = logicalDevice.getQueue(indices.graphicsFamily.value(), 0);
    presentQueue = logicalDevice.getQueue(indices.presentFamily.value(), 0);
    computeQueue = logicalDevice.getQueue(indices.computeFamily.value(), 0);

    auto allocatorFlags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;

    auto allocatorCreateInfo = VmaAllocatorCreateInfo{.flags = allocatorFlags,
                                                      .physicalDevice = **physicalDevice,
                                                      .device = *get(),
                                                      .pAllocationCallbacks{},
                                                      .pDeviceMemoryCallbacks{},
                                                      .pHeapSizeLimit{},
                                                      .instance = **instance,
                                                      .vulkanApiVersion = vk::ApiVersion14,
                                                      .pTypeExternalMemoryHandleTypes{}};

    vmaCreateAllocator(&allocatorCreateInfo, &memoryAllocator);
}

auto Vulkan::Device::get() -> const vk::raii::Device& { return logicalDevice; }

auto Vulkan::Device::getPhysicalDevice() -> const vk::raii::PhysicalDevice* { return physicalDevice; }

auto Vulkan::Device::getGraphicsQueue() -> const vk::raii::Queue& { return graphicsQueue; }

auto Vulkan::Device::getPresentQueue() -> const vk::raii::Queue& { return presentQueue; }

auto Vulkan::Device::getAllocator() -> const VmaAllocator& { return memoryAllocator; }
