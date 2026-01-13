#include "render.hpp"

#include "commandPool.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "deviceBuffer.hpp"
#include "frameSync.hpp"
#include "gpuDataObjects.hpp"
#include "graphicsPipeline.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "shaderDef.hpp"
#include "surface.hpp"
#include "uploadContext.hpp"
#include "vertex.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/common.hpp>
#include <vector>

Render::Render::Render(SDL_Window* window, const uint32_t maxFramesInFlight)
    : maxFramesInFlight(maxFramesInFlight), window(window), instance("Vulkan prototype", window),
      surface(window, instance), physicalDevice(instance, surface), device(physicalDevice, instance),
      commandPool(physicalDevice, device, maxFramesInFlight), descriptorPool(device, maxFramesInFlight * 2),
      swapchain(physicalDevice, device, surface, window, vk::SampleCountFlagBits::e4),
      syncObjects(device, swapchain.getImages().size()),
      stagingBuffer(device, static_cast<vk::DeviceSize>(5 * 1024 * 1024)) {}

void Render::Render::uploadScene(const Scene& scene) {
    size_t totalVertices = 0;
    size_t totalIndices = 0;

    for (const auto* model : scene.getObjects()) {
        totalVertices += model->vertices.size();
        totalIndices += model->indices.size();
    }

    vertexBuffer.emplace(
        Vulkan::DeviceBuffer(device,
                             totalVertices * sizeof(Vertex),
                             vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst));
    indexBuffer.emplace(
        Vulkan::DeviceBuffer(device,
                             totalIndices * sizeof(uint32_t),
                             vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst));

    /* SHOULD BE MATERIAL DEPENDENT */
    textures.reserve(1);
    textures.emplace_back(device, "/home/bence/Documents/CodingProjects/C++/vulkan-sdl/models/viking_room.png");

    const ShaderDef vertexShader{.filePath =
                                     "/home/bence/Documents/CodingProjects/C++/vulkan-sdl/src/shaders/vert.glsl",
                                 .stage = EShLangVertex};
    const ShaderDef fragmentShader{.filePath =
                                       "/home/bence/Documents/CodingProjects/C++/vulkan-sdl/src/shaders/frag.glsl",
                                   .stage = EShLangFragment};

    {
        auto context = Vulkan::UploadContext(device, commandPool, stagingBuffer);
        uint32_t vertexOffset = 0;
        uint32_t indexOffset = 0;

        for (const auto* model : scene.getObjects()) {
            context.upload(vertexBuffer.value(),
                           model->vertices.data(),
                           model->vertices.size() * sizeof(Vertex),
                           vertexOffset * sizeof(Vertex));

            context.upload(indexBuffer.value(),
                           model->indices.data(),
                           model->indices.size() * sizeof(uint32_t),
                           indexOffset * sizeof(uint32_t));

            vertexOffset += model->vertices.size();
            indexOffset += model->indices.size();
        }

        context.upload(*textures.data());
    }

    uniformBuffers.reserve(maxFramesInFlight);
    for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
        uniformBuffers.emplace_back(device, sizeof(UniformBufferObject));
    }

    cameraDescriptorSetLayout.emplace(
        Vulkan::DescriptorSetLayout(device,
                                    std::vector<vk::DescriptorSetLayoutBinding>{vk::DescriptorSetLayoutBinding(
                                        0,
                                        vk::DescriptorType::eUniformBuffer,
                                        1,
                                        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)}));

    materialDescriptorSetLayout.emplace(
        Vulkan::DescriptorSetLayout(device,
                                    std::vector<vk::DescriptorSetLayoutBinding>{
                                        vk::DescriptorSetLayoutBinding(0,
                                                                       vk::DescriptorType::eCombinedImageSampler,
                                                                       1,
                                                                       vk::ShaderStageFlagBits::eFragment,
                                                                       &*textures.data()->getSampler())}));

    cameraDescriptorSets.reserve(maxFramesInFlight);

    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        cameraDescriptorSets.emplace_back(device, descriptorPool, cameraDescriptorSetLayout.value());
        const DescriptorWrite uboWrite{.binding = 0,
                                       .type = vk::DescriptorType::eUniformBuffer,
                                       .bufferInfo = {uniformBuffers[i].get(), 0, sizeof(UniformBufferObject)}};
        cameraDescriptorSets[i].write(std::span(&uboWrite, 1));
    }

    materialDescriptorSets.reserve(maxFramesInFlight);

    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        materialDescriptorSets.emplace_back(device, descriptorPool, materialDescriptorSetLayout.value());
        const DescriptorWrite materialWrite{.binding = 0,
                                            .type = vk::DescriptorType::eCombinedImageSampler,
                                            .imageInfo{textures[0].getSampler(),
                                                       textures[0].getImage().getImageView(),
                                                       vk::ImageLayout::eShaderReadOnlyOptimal}};
        materialDescriptorSets[i].write(std::span(&materialWrite, 1));
    }

    std::array<const Vulkan::DescriptorSetLayout*, 2> layouts = {&cameraDescriptorSetLayout.value(),
                                                                 &materialDescriptorSetLayout.value()};
    pipeline.emplace(Vulkan::GraphicsPipeline(device, swapchain, layouts, vertexShader, fragmentShader));
}

void Render::Render::render(Scene& scene, float deltaTime) {
    syncObjects.waitForFrame(frameIndex);

    auto [result, imageIndex] = swapchain.get().acquireNextImage(UINT64_MAX, syncObjects.getImageAvailable(frameIndex));

    if (result == vk::Result::eErrorOutOfDateKHR) {
        swapchain.recreate(device, surface, window);
    }

    syncObjects.resetFence(frameIndex);

    const auto cmd = *commandPool.getCommandBuffer(frameIndex);

    cmd.reset();

    const vk::CommandBufferBeginInfo beginInfo;

    cmd.begin(beginInfo);

    // Transition framebuffer for dynamic rendering
    Vulkan::CommandPool::transitionImage(swapchain.getImage(imageIndex),
                                         vk::ImageLayout::eColorAttachmentOptimal,
                                         vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                         vk::AccessFlagBits2::eColorAttachmentWrite,
                                         vk::ImageAspectFlagBits::eColor,
                                         cmd);

    // Transition MSAA buffer
    Vulkan::CommandPool::transitionImage(swapchain.getMsaaImage(),
                                         vk::ImageLayout::eColorAttachmentOptimal,
                                         vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                         vk::AccessFlagBits2::eColorAttachmentWrite,
                                         vk::ImageAspectFlagBits::eColor,
                                         cmd);

    // Transition depth buffer for dynamic rendering (it doesn't need to be done again, unless it's read!)
    Vulkan::CommandPool::transitionImage(swapchain.getDepthImage(),
                                         vk::ImageLayout::eDepthAttachmentOptimal,
                                         vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                             vk::PipelineStageFlagBits2::eLateFragmentTests,
                                         vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                                         vk::ImageAspectFlagBits::eDepth,
                                         cmd);
    // Begin render
    const vk::ClearValue clearColor{std::array<float, 4>{0.2f, 0.4f, 1.0f, 1.0f}};
    const vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    const vk::RenderingAttachmentInfo colorAttachmentInfo(swapchain.getMsaaImage().getImageView(),
                                                          vk::ImageLayout::eColorAttachmentOptimal,
                                                          vk::ResolveModeFlagBits::eAverage,
                                                          swapchain.getImage(imageIndex).getImageView(),
                                                          vk::ImageLayout::eColorAttachmentOptimal,
                                                          vk::AttachmentLoadOp::eClear,
                                                          vk::AttachmentStoreOp::eStore,
                                                          clearColor);

    const vk::RenderingAttachmentInfo depthAttachmentInfo(swapchain.getDepthImage().getImageView(),
                                                          vk::ImageLayout::eDepthAttachmentOptimal,
                                                          vk::ResolveModeFlagBits::eNone,
                                                          {},
                                                          vk::ImageLayout::eUndefined,
                                                          vk::AttachmentLoadOp::eClear,
                                                          vk::AttachmentStoreOp::eStore,
                                                          clearDepth);

    const vk::RenderingInfo renderInfo(
        {}, {{0, 0}, swapchain.getExtent()}, 1, {}, 1, &colorAttachmentInfo, &depthAttachmentInfo);

    cmd.beginRendering(renderInfo);

    // Bind the graphics pipeline
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->get());

    const vk::Viewport viewport(0.0,
                                1.0,
                                static_cast<float>(swapchain.getExtent().width),
                                static_cast<float>(swapchain.getExtent().height),
                                0.0,
                                1.0);

    cmd.setViewport(0, 1, &viewport);

    const vk::Rect2D scissor(VkOffset2D(0), swapchain.getExtent());
    cmd.setScissor(0, 1, &scissor);

    const vk::Buffer vertexBuffers[] = {vertexBuffer->get()};

    // Draw
    UniformBufferObject ubo{};
    ubo.view = scene.getCamera().getViewMatrix();
    ubo.proj = glm::perspective(
        glm::radians(45.0f), (float)swapchain.getExtent().width / (float)swapchain.getExtent().height, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;

    uniformBuffers[frameIndex].update(&ubo, sizeof(UniformBufferObject));

    // per model
    for (const auto* model : scene.getObjects()) {
        const vk::DeviceSize offsets[] = {0};
        cmd.bindVertexBuffers(0, vertexBuffers, offsets);
        cmd.bindIndexBuffer(indexBuffer->get(), 0, vk::IndexType::eUint32);
        const PushConstants pc{model->getModelMatrix()};
        cmd.pushConstants(
            pipeline->getLayout(), vk::ShaderStageFlagBits::eVertex, 0, vk::ArrayProxy<const PushConstants>(1, &pc));
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               pipeline->getLayout(),
                               0,
                               {*cameraDescriptorSets[frameIndex].get(), *materialDescriptorSets[frameIndex].get()},
                               nullptr);
        cmd.drawIndexed(model->indices.size(), 1, 0, 0, 0);
    }

    cmd.endRendering();
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

    device.getGraphicsQueue().submit(submitInfo, syncObjects.getFence(frameIndex));

    vk::SwapchainKHR swapchains[] = {swapchain.get()};
    const vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, swapchains, &imageIndex);

    result = device.getPresentQueue().presentKHR(presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        swapchain.recreate(device, surface, window);
    }

    frameIndex = (frameIndex + 1) % maxFramesInFlight;
}