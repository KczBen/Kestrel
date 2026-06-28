#include "commandPool.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
CommandPool::CommandPool(PhysicalDevice& physicalDevice, Device& device, uint32_t maxFramesInFlight)
    : device(&*device), graphicsQueue(device.getGraphicsQueue()) {
    const auto& indices = physicalDevice.getQueueFamilyIndices();
    if (!indices.computeFamily.has_value()) {
        throw std::runtime_error("Compute queue family not found");
    }
    const vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                    indices.computeFamily.value());

    pool = vk::raii::CommandPool(*this->device, commandPoolInfo);

    const vk::CommandBufferAllocateInfo commandBufferInfo(*pool, vk::CommandBufferLevel::ePrimary, maxFramesInFlight);

    buffers = vk::raii::CommandBuffers(*this->device, commandBufferInfo);
}

void CommandPool::transitionImage(Image& image,
                                  vk::ImageLayout newLayout,
                                  vk::PipelineStageFlags2 dstStage,
                                  vk::AccessFlags2 dstAccess,
                                  vk::ImageAspectFlags dstAspect,
                                  vk::CommandBuffer cmd) {

    if (image.layout == newLayout && image.stageMask == dstStage && image.accessFlags == dstAccess) {
        return;
    }

    const vk::ImageMemoryBarrier2 barrier{image.stageMask,
                                          image.accessFlags,
                                          dstStage,
                                          dstAccess,
                                          image.layout,
                                          newLayout,
                                          vk::QueueFamilyIgnored,
                                          vk::QueueFamilyIgnored,
                                          image.image,
                                          vk::ImageSubresourceRange(dstAspect, 0, image.mipLevelCount, 0, 1)};

    vk::DependencyInfo depInfo{};
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    cmd.pipelineBarrier2(depInfo);

    image.layout = newLayout;
    image.stageMask = dstStage;
    image.accessFlags = dstAccess;
    image.aspectFlags = dstAspect;
}

void CommandPool::transitionImages(std::span<ImageTransitionInfo> transitions, vk::CommandBuffer cmd) {
    std::vector<vk::ImageMemoryBarrier2> barriers;

    barriers.reserve(transitions.size());
    for (auto transition : transitions) {
        if (transition.image->layout == transition.newLayout && transition.image->stageMask == transition.dstStage &&
            transition.image->accessFlags == transition.dstAccess) {
            continue;
        }

        barriers.emplace_back(
            transition.image->stageMask,
            transition.image->accessFlags,
            transition.dstStage,
            transition.dstAccess,
            transition.image->layout,
            transition.newLayout,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            transition.image->image,
            vk::ImageSubresourceRange(transition.dstAspect, 0, transition.image->mipLevelCount, 0, 1));

        transition.image->layout = transition.newLayout;
        transition.image->stageMask = transition.dstStage;
        transition.image->accessFlags = transition.dstAccess;
        transition.image->aspectFlags = transition.dstAspect;
    }

    vk::DependencyInfo depInfo{};
    depInfo.imageMemoryBarrierCount = barriers.size();
    depInfo.pImageMemoryBarriers = barriers.data();

    cmd.pipelineBarrier2(depInfo);
}

void CommandPool::generateMipmaps(Image& image, vk::CommandBuffer cmd) {
    vk::ImageMemoryBarrier2 barrier(vk::PipelineStageFlagBits2::eTransfer,
                                    vk::AccessFlagBits2::eTransferWrite,
                                    vk::PipelineStageFlagBits2::eBlit,
                                    vk::AccessFlagBits2::eTransferWrite,
                                    vk::ImageLayout::eTransferDstOptimal,
                                    vk::ImageLayout::eTransferSrcOptimal,
                                    vk::QueueFamilyIgnored,
                                    vk::QueueFamilyIgnored,
                                    image.image,
                                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

    auto mipWidth = static_cast<int32_t>(image.imageExtent.width);
    auto mipHeight = static_cast<int32_t>(image.imageExtent.height);

    vk::DependencyInfo depInfo{};
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    for (uint32_t i = 1; i < image.mipLevelCount; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead;

        cmd.pipelineBarrier2(depInfo);

        vk::ArrayWrapper1D<vk::Offset3D, 2> offsets;
        vk::ArrayWrapper1D<vk::Offset3D, 2> dstOffsets;
        offsets[0] = vk::Offset3D(0, 0, 0);
        offsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
        dstOffsets[0] = vk::Offset3D(0, 0, 0);
        dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);
        const vk::ImageBlit2 blit(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1),
                                  offsets,
                                  vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i, 0, 1),
                                  dstOffsets);

        const vk::BlitImageInfo2 blitInfo(*image,
                                          vk::ImageLayout::eTransferSrcOptimal,
                                          *image,
                                          vk::ImageLayout::eTransferDstOptimal,
                                          1,
                                          &blit,
                                          vk::Filter::eLinear);

        cmd.blitImage2(blitInfo);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eCopy;
        // depInfo still points to the barrier, which was updated since, so this is fine
        cmd.pipelineBarrier2(depInfo);

        if (mipWidth > 1) {
            mipWidth /= 2;
        }
        if (mipHeight > 1) {
            mipHeight /= 2;
        }
    }

    barrier.subresourceRange.baseMipLevel = image.mipLevelCount - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;

    cmd.pipelineBarrier2(depInfo);
}

auto CommandPool::getCommandBuffer(uint32_t frame) -> const vk::raii::CommandBuffer& { return buffers[frame]; }
auto CommandPool::get() -> const vk::raii::CommandPool& { return pool; }
} // namespace Vulkan