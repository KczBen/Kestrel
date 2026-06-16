#include "render.hpp"

#include "cameraPass.hpp"
#include "commandPool.hpp"
#include "frameSync.hpp"
#include "geometryPass.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "material.hpp"
#include "physicalDevice.hpp"
#include "resourceManager.hpp"
#include "surface.hpp"
#include "vulkan/vulkan.hpp"
#include <SDL3/SDL_filesystem.h>
#include <array>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <memory>
#include <vector>

Render::Render::Render(SDL_Window* window,
                       ResourceManager& resources,
                       Vulkan::Instance& instance,
                       Vulkan::Surface& surface,
                       Vulkan::PhysicalDevice& physicalDevice,
                       Vulkan::Device& device,
                       const uint32_t maxFramesInFlight)
    : maxFramesInFlight(maxFramesInFlight), window(window), instance(&instance), surface(&surface),
      physicalDevice(&physicalDevice), device(&device), resourceManager(&resources),
      commandPool(physicalDevice, device, maxFramesInFlight),
      swapchain(physicalDevice, device, surface, getWindowSizeInPixels(window), vk::SampleCountFlagBits::e4),
      syncObjects(device, swapchain.getImages().size()) {}

Render::Render::Render(ResourceManager& resources,
                       Vulkan::Instance& instance,
                       Vulkan::Surface& surface,
                       Vulkan::PhysicalDevice& physicalDevice,
                       Vulkan::Device& device,
                       const uint32_t maxFramesInFlight,
                       const uint32_t width,
                       const uint32_t height)
    : maxFramesInFlight(maxFramesInFlight), window(nullptr), instance(&instance), surface(&surface),
      physicalDevice(&physicalDevice), device(&device), resourceManager(&resources),
      commandPool(physicalDevice, device, maxFramesInFlight),
      swapchain(physicalDevice, device, surface, {width, height}, vk::SampleCountFlagBits::e4),
      syncObjects(device, swapchain.getImages().size()), width(width), height(height) {}

void Render::Render::init(Scene& scene) {
    passes.emplace_back(std::make_unique<CameraPass>(CameraPass(
        resourceManager->getPipelineManager(), {&resourceManager->getMaterialList(), 0}, &scene.getCamera())));

    passes.emplace_back(std::make_unique<GeometryPass>(
        GeometryPass(resourceManager->getPipelineManager(), scene.getObjects(), &scene.getCamera())));
}

void Render::Render::render(Scene& scene, float deltaTime) {
    syncObjects.waitForFrame(frameIndex);

    auto [result, imageIndex] = swapchain.get().acquireNextImage(UINT64_MAX, syncObjects.getImageAvailable(frameIndex));

    if (result == vk::Result::eErrorOutOfDateKHR) {
        auto [w, h] = getCurrentExtent();
        swapchain.recreate(*device, *surface, w, h);
    }

    syncObjects.resetFence(frameIndex);

    auto& target = swapchain.getRenderTarget(imageIndex);

    const auto cmd = *commandPool.getCommandBuffer(frameIndex);

    cmd.reset();

    const vk::CommandBufferBeginInfo beginInfo;

    cmd.begin(beginInfo);

    // Transition framebuffer for dynamic rendering
    Vulkan::CommandPool::transitionImage(target.getColorImage(),
                                         vk::ImageLayout::eColorAttachmentOptimal,
                                         vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                         vk::AccessFlagBits2::eColorAttachmentWrite,
                                         vk::ImageAspectFlagBits::eColor,
                                         cmd);

    // Transition MSAA buffer
    Vulkan::CommandPool::transitionImage(target.getMsaaImage(),
                                         vk::ImageLayout::eColorAttachmentOptimal,
                                         vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                         vk::AccessFlagBits2::eColorAttachmentWrite,
                                         vk::ImageAspectFlagBits::eColor,
                                         cmd);

    // Transition depth buffer for dynamic rendering (it doesn't need to be done again, unless it's read!)
    Vulkan::CommandPool::transitionImage(target.getDepthImage(),
                                         vk::ImageLayout::eDepthAttachmentOptimal,
                                         vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                             vk::PipelineStageFlagBits2::eLateFragmentTests,
                                         vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                                         vk::ImageAspectFlagBits::eDepth,
                                         cmd);
    // Begin render
    const vk::ClearValue clearColor{std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}};
    const vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    const vk::RenderingAttachmentInfo colorAttachmentInfo(target.getMsaaImage().getImageView(),
                                                          vk::ImageLayout::eColorAttachmentOptimal,
                                                          vk::ResolveModeFlagBits::eAverage,
                                                          target.getColorImage().getImageView(),
                                                          vk::ImageLayout::eColorAttachmentOptimal,
                                                          vk::AttachmentLoadOp::eClear,
                                                          vk::AttachmentStoreOp::eStore,
                                                          clearColor);

    const vk::RenderingAttachmentInfo depthAttachmentInfo(target.getDepthImage().getImageView(),
                                                          vk::ImageLayout::eDepthAttachmentOptimal,
                                                          vk::ResolveModeFlagBits::eNone,
                                                          {},
                                                          vk::ImageLayout::eUndefined,
                                                          vk::AttachmentLoadOp::eClear,
                                                          vk::AttachmentStoreOp::eStore,
                                                          clearDepth);

    const vk::RenderingInfo renderInfo(
        {}, {{0, 0}, target.getExtent()}, 1, {}, 1, &colorAttachmentInfo, &depthAttachmentInfo);

    cmd.beginRendering(renderInfo);

    const vk::Viewport viewport(0.0,
                                1.0,
                                static_cast<float>(target.getExtent().width),
                                static_cast<float>(target.getExtent().height),
                                0.0,
                                1.0);

    cmd.setViewport(0, 1, &viewport);

    const vk::Rect2D scissor(VkOffset2D(0), target.getExtent());
    cmd.setScissor(0, 1, &scissor);

    // Draw
    for (auto& pass : passes) {
        pass->record(cmd, frameIndex);
    }

    cmd.endRendering();
    Vulkan::CommandPool::transitionImage(target.getColorImage(),
                                         vk::ImageLayout::ePresentSrcKHR,
                                         vk::PipelineStageFlagBits2::eBottomOfPipe,
                                         {},
                                         vk::ImageAspectFlagBits::eColor,
                                         cmd);
    cmd.end();

    vk::Semaphore waitSemaphores[] = {syncObjects.getImageAvailable(frameIndex)};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::Semaphore signalSemaphores[] = {syncObjects.getRenderFinished(imageIndex)};

    const vk::SubmitInfo submitInfo(1, waitSemaphores, waitStages, 1, &cmd, 1, signalSemaphores);

    device->getGraphicsQueue().submit(submitInfo, syncObjects.getFence(frameIndex));

    vk::SwapchainKHR swapchains[] = {swapchain.get()};
    const vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, swapchains, &imageIndex);

    result = device->getPresentQueue().presentKHR(presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        auto [w, h] = getCurrentExtent();
        swapchain.recreate(*device, *surface, w, h);
    }

    frameIndex = (frameIndex + 1) % maxFramesInFlight;
}
