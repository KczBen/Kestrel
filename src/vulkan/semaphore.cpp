#include "semaphore.hpp"
#include "device.hpp"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_raii.hpp>

Vulkan::Semaphore::Semaphore(Vulkan::Device& device) : device(&device.get()) {
    const vk::SemaphoreCreateInfo semaphoreInfo((vk::SemaphoreCreateFlags()));

    semaphore = vk::raii::Semaphore(*this->device, semaphoreInfo);
}

auto Vulkan::Semaphore::get() -> const vk::raii::Semaphore& { return semaphore; }