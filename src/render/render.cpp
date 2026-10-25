#include "render.hpp"

#include "cameraPass.hpp"
#include "frameGraph.hpp"
#include "geometryPass.hpp"
#include "imguiPass.hpp"
#include "material.hpp"
#include "pass.hpp"
#include "resourceManager.hpp"
#include "vulkan-wrapper/commandPool.hpp"
#include "vulkan-wrapper/frameSync.hpp"
#include "vulkan-wrapper/image.hpp"
#include "vulkan-wrapper/instance.hpp"
#include "vulkan-wrapper/physicalDevice.hpp"
#include "vulkan-wrapper/surface.hpp"

#include <SDL3/SDL_filesystem.h>
#include <array>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

Render::Render::Render(SDL_Window* window,
                       ResourceManager& resources,
                       Vulkan::Instance& instance,
                       Vulkan::Surface& surface,
                       Vulkan::PhysicalDevice& physicalDevice,
                       Vulkan::Device& device,
                       const uint32_t maxFramesInFlight)
    : maxFramesInFlight(maxFramesInFlight), window(window), instance(&instance), surface(&surface),
      physicalDevice(&physicalDevice), device(&device), resources(&resources),
      commandPool(physicalDevice, device, maxFramesInFlight),
      swapchain(physicalDevice, device, surface, getWindowSizeInPixels(window)),
      syncObjects(device, swapchain.getImages().size()), height(swapchain.getExtent().height),
      width(swapchain.getExtent().width), frameGraph(resources.getPipelineManager()) {}

Render::Render::Render(ResourceManager& resources,
                       Vulkan::Instance& instance,
                       Vulkan::Surface& surface,
                       Vulkan::PhysicalDevice& physicalDevice,
                       Vulkan::Device& device,
                       const uint32_t maxFramesInFlight,
                       const uint32_t width,
                       const uint32_t height)
    : maxFramesInFlight(maxFramesInFlight), window(nullptr), instance(&instance), surface(&surface),
      physicalDevice(&physicalDevice), device(&device), resources(&resources),
      commandPool(physicalDevice, device, maxFramesInFlight),
      swapchain(physicalDevice, device, surface, {width, height}), syncObjects(device, swapchain.getImages().size()),
      width(width), height(height), frameGraph(resources.getPipelineManager()) {}

void Render::Render::init(Scene::Scene& scene) {
    const auto& swapchainExtent = swapchain.getExtent();

    colorTarget =
        resources->createRenderTarget("FRAMEBUFFER",
                                      vk::Extent3D{swapchainExtent.width, swapchainExtent.height, 1},
                                      vk::Format::eB8G8R8A8Srgb,
                                      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                                      vk::ImageAspectFlagBits::eColor);

    depthTarget = resources->createRenderTarget("DEPTH",
                                                vk::Extent3D{swapchainExtent.width, swapchainExtent.height, 1},
                                                vk::Format::eD32Sfloat,
                                                vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                vk::ImageAspectFlagBits::eDepth);

    frameGraph.addPass(std::make_unique<GeometryPass>(
        GeometryPass("depth_pre",
                     {},
                     {},
                     {},
                     {depthTarget, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore},
                     {},
                     resources->getMaterial("depth_only"))));

    frameGraph.addPass(std::make_unique<CameraPass>(
        CameraPass("sky",
                   {},
                   {{colorTarget, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore}},
                   {depthTarget},
                   {},
                   {},
                   resources->getMaterial("sky"))));

    frameGraph.addPass(std::make_unique<GeometryPass>(
        GeometryPass("geometry",
                     {},
                     {{colorTarget, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore}},
                     {depthTarget},
                     {},
                     {},
                     std::nullopt)));

    frameGraph.addPass(std::make_unique<CameraPass>(
        CameraPass("grid_overlay",
                   {},
                   {{colorTarget, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore}},
                   {depthTarget},
                   {},
                   {},
                   resources->getMaterial("grid_overlay"))));

    // frameGraph.addPass(std::make_unique<ImGuiPass>(
    //     ImGuiPass("gui", {colorTarget, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore}, {"sky"})));

    frameGraph.compile();
}

void Render::Render::render(Scene::Scene& scene, float deltaTime) {
    syncObjects.waitForFrame(frameIndex);

    auto [result, imageIndex] = (*swapchain).acquireNextImage(UINT64_MAX, syncObjects.getImageAvailable(frameIndex));

    if (result == vk::Result::eErrorOutOfDateKHR) {
        auto [w, h] = getCurrentExtent();
        swapchain.recreate(*device, *surface, w, h);
    }

    syncObjects.resetFence(frameIndex);

    auto& colorImage = *colorTarget;
    auto& depthImage = *depthTarget;
    const auto& extent = swapchain.getExtent();

    const auto cmd = *commandPool.getCommandBuffer(frameIndex);

    cmd.reset();

    const vk::CommandBufferBeginInfo beginInfo;

    cmd.begin(beginInfo);

    // Transition color render target for dynamic rendering
    Vulkan::CommandPool::transitionImage(colorImage,
                                         vk::ImageLayout::eColorAttachmentOptimal,
                                         vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                         vk::AccessFlagBits2::eColorAttachmentWrite,
                                         vk::ImageAspectFlagBits::eColor,
                                         cmd);

    // Transition depth render target for dynamic rendering
    Vulkan::CommandPool::transitionImage(depthImage,
                                         vk::ImageLayout::eDepthAttachmentOptimal,
                                         vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                             vk::PipelineStageFlagBits2::eLateFragmentTests,
                                         vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                                         vk::ImageAspectFlagBits::eDepth,
                                         cmd);

    // Execute frame graph
    frameGraph.execute(cmd, resources->getResources());

    // Transition render target to transfer src
    Vulkan::CommandPool::transitionImage(colorImage,
                                         vk::ImageLayout::eTransferSrcOptimal,
                                         vk::PipelineStageFlagBits2::eAllGraphics,
                                         vk::AccessFlagBits2::eTransferRead,
                                         vk::ImageAspectFlagBits::eColor,
                                         cmd);

    // Transition swapchain image to transfer dst
    Vulkan::CommandPool::transitionImage(swapchain.getImage(imageIndex),
                                         vk::ImageLayout::eTransferDstOptimal,
                                         vk::PipelineStageFlagBits2::eBottomOfPipe,
                                         {},
                                         vk::ImageAspectFlagBits::eColor,
                                         cmd);

    // Blit render target to swapchain image
    const vk::ImageBlit blitInfo(
        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
        std::array{vk::Offset3D(0, 0, 0),
                   vk::Offset3D(static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1)},
        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
        std::array{vk::Offset3D(0, 0, 0),
                   vk::Offset3D(static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1)});

    cmd.blitImage(*colorImage,
                  vk::ImageLayout::eTransferSrcOptimal,
                  *swapchain.getImage(imageIndex),
                  vk::ImageLayout::eTransferDstOptimal,
                  {blitInfo},
                  vk::Filter::eLinear);

    // Transition swapchain image to present
    Vulkan::CommandPool::transitionImage(swapchain.getImage(imageIndex),
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

    vk::SwapchainKHR swapchains[] = {*swapchain};
    const vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, swapchains, &imageIndex);

    result = device->getPresentQueue().presentKHR(presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        auto [w, h] = getCurrentExtent();
        swapchain.recreate(*device, *surface, w, h);
    }

    frameIndex = (frameIndex + 1) % maxFramesInFlight;
}