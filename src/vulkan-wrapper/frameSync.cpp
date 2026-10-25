#include "frameSync.hpp"
#include "device.hpp"

Vulkan::FrameSync::FrameSync(Vulkan::Device& device, uint32_t maxFramesInFlight) : device(*device) {
    const vk::FenceCreateFlags flags(vk::FenceCreateFlagBits::eSignaled);

    inFlightFences.reserve(maxFramesInFlight);
    imageAvailable.reserve(maxFramesInFlight);
    renderFinished.reserve(maxFramesInFlight);

    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        inFlightFences.emplace_back(device, flags);
        imageAvailable.emplace_back(device);
        renderFinished.emplace_back(device);
    }
}

void Vulkan::FrameSync::waitForFrame(uint32_t frame) {
    auto res = this->device.waitForFences(**inFlightFences[frame], vk::True, 0xFFFFFFFFFFFFFFFF);
}

void Vulkan::FrameSync::resetFence(uint32_t frame) { this->device.resetFences(**inFlightFences[frame]); }

auto Vulkan::FrameSync::getImageAvailable(uint32_t frame) -> const vk::raii::Semaphore& {
    return *imageAvailable[frame];
}

auto Vulkan::FrameSync::getRenderFinished(uint32_t frame) -> const vk::raii::Semaphore& {
    return *renderFinished[frame];
}

auto Vulkan::FrameSync::getFence(uint32_t frame) -> const vk::raii::Fence& { return *inFlightFences[frame]; }
