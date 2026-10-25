#pragma once

#include "device.hpp"
#include "fence.hpp"
#include "semaphore.hpp"

namespace Vulkan {
class FrameSync {
  private:
    vk::Device device;
    std::vector<Fence> inFlightFences;
    std::vector<Semaphore> imageAvailable;
    std::vector<Semaphore> renderFinished;

  public:
    FrameSync(Vulkan::Device& device, uint32_t maxFramesInFlight);

    void waitForFrame(uint32_t frame);
    void resetFence(uint32_t frame);

    auto getImageAvailable(uint32_t frame) -> const vk::raii::Semaphore&;
    auto getRenderFinished(uint32_t frame) -> const vk::raii::Semaphore&;
    auto getFence(uint32_t frame) -> const vk::raii::Fence&;
};
} // namespace Vulkan