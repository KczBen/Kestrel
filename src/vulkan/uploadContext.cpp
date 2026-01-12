#include "uploadContext.hpp"
#include "commandPool.hpp"
#include "vulkan/vulkan.hpp"

Vulkan::UploadContext::UploadContext(Vulkan::Device& device, Vulkan::CommandPool& pool, UploadBuffer& staging)
    : device(&device.get()), queue(&device.getGraphicsQueue()), pool(&pool.get()), staging(&staging) {
    const vk::CommandBufferAllocateInfo allocInfo(*this->pool, vk::CommandBufferLevel::ePrimary, 1);

    vk::raii::CommandBuffers buffers(*this->device, allocInfo);
    cmd = std::move(buffers[0]);

    const vk::CommandBufferBeginInfo begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmd.begin(begin);
}

void Vulkan::UploadContext::upload(DeviceBuffer& dst, const void* data, vk::DeviceSize size, vk::DeviceSize offset) {
    auto alloc = staging->allocate(size, 16);
    std::memcpy(alloc.cpuPtr, data, size);

    const vk::BufferCopy copy(alloc.offset, offset, size);
    cmd.copyBuffer(staging->get(), dst.get(), copy);
}

void Vulkan::UploadContext::upload(Texture& texture) {
    assert(!texture.uploaded);

    const auto size = static_cast<vk::DeviceSize>(texture.pixelData.size());

    auto alloc = staging->allocate(size, 4);
    std::memcpy(alloc.cpuPtr, texture.pixelData.data(), size);

    const vk::BufferImageCopy region(alloc.offset,
                                     0,
                                     0,
                                     vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                     vk::Offset3D(0, 0, 0),
                                     texture.extent);

    Vulkan::CommandPool::transitionImage(texture.image,
                                         vk::ImageLayout::eTransferDstOptimal,
                                         vk::PipelineStageFlagBits2::eTransfer,
                                         vk::AccessFlagBits2::eTransferWrite,
                                         vk::ImageAspectFlagBits::eColor,
                                         cmd);

    cmd.copyBufferToImage(staging->get(), texture.image.image, vk::ImageLayout::eTransferDstOptimal, region);

    Vulkan::CommandPool::generateMipmaps(texture.image, cmd);

    // Vulkan::CommandPool::transitionImage(texture.image,
    //                                      vk::ImageLayout::eShaderReadOnlyOptimal,
    //                                      vk::PipelineStageFlagBits2::eFragmentShader,
    //                                      vk::AccessFlagBits2::eShaderRead,
    //                                      vk::ImageAspectFlagBits::eColor,
    //                                      cmd);

    texture.uploaded = true;
    texture.pixelData.clear();
}

void Vulkan::UploadContext::flushAndWait() {
    cmd.end();

    const vk::SubmitInfo submit({}, {}, {}, 1, &*cmd);
    queue->submit(submit);
    queue->waitIdle();

    staging->reset();
}

// void Vulkan::UploadContext::transitionImage(Image& image,
//                                             vk::ImageLayout newLayout,
//                                             vk::PipelineStageFlags2 dstStage,
//                                             vk::AccessFlags2 dstAccess) {
//     const vk::ImageMemoryBarrier2 barrier{
//         image.stageMask,
//         image.accessFlags,
//         dstStage,
//         dstAccess,
//         image.layout,
//         newLayout,
//         VK_QUEUE_FAMILY_IGNORED,
//         VK_QUEUE_FAMILY_IGNORED,
//         image.image,
//         vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, image.mipLevelCount, 0, 1)};
//
//     vk::DependencyInfo depInfo{};
//     depInfo.imageMemoryBarrierCount = 1;
//     depInfo.pImageMemoryBarriers = &barrier;
//
//     cmd.pipelineBarrier2(depInfo);
//
//     image.layout = newLayout;
//     image.stageMask = dstStage;
//     image.accessFlags = dstAccess;
// }
