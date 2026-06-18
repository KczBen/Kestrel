#include "texture.hpp"
#include "image.hpp"
#include "sampler.hpp"
#include <vulkan/vulkan_raii.hpp>

#include "stb_image.h"

Vulkan::Texture::Texture(
    Vulkan::Device& device, const void* data, uint32_t width, uint32_t height, SamplerHandle sampler, vk::Format format)
    : format(format), samplerHandle(sampler) {
    const auto dataSize = width * height * 4;
    pixelData.resize(dataSize);
    std::memcpy(pixelData.data(), data, dataSize);

    extent = vk::Extent3D(width, height, 1);

    image = Image(device,
                  extent,
                  format,
                  vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
                      vk::ImageUsageFlagBits::eSampled,
                  vk::ImageAspectFlagBits::eColor);
}

auto Vulkan::Texture::getImage() -> Vulkan::Image& { return image; }

auto Vulkan::Texture::getSampler() -> const vk::raii::Sampler& { return samplerHandle.get(); }
