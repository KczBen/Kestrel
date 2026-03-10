#include "instance.hpp"

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

Vulkan::Instance::Instance(const std::string& appName, std::span<const char* const> extensions) {
    const vk::ApplicationInfo appInfo(appName.c_str(),
                                      vk::makeApiVersion(0, 1, 0, 0),
                                      "Proto Engine",
                                      vk::makeApiVersion(0, 1, 0, 0),
                                      vk::ApiVersion14);

    // Validation layers
    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    vk::InstanceCreateInfo instanceInfo{};
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();

    // Check if validation layers are available
    auto availableLayers = vk::enumerateInstanceLayerProperties();
    bool validationAvailable = false;
    for (const auto& layerProperties : availableLayers) {
        if (strcmp("VK_LAYER_KHRONOS_validation", layerProperties.layerName) == 0) {
            validationAvailable = true;
            break;
        }
    }

    instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    instanceInfo.ppEnabledLayerNames = validationLayers.data();

    instance = vk::raii::Instance(context, instanceInfo);
}

auto Vulkan::Instance::get() -> vk::raii::Instance& { return instance; }