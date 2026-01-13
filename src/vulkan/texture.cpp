#include "texture.hpp"
#include "image.hpp"
#include <vulkan/vulkan_raii.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

Vulkan::Texture::Texture(Device& device, const char* texturePath) {
    int w = 0;
    int h = 0;
    int c = 0;

    stbi_uc* pixels = stbi_load(texturePath, &w, &h, &c, STBI_rgb_alpha);

    if (pixels == nullptr) {
        throw std::runtime_error("Failed to load texture");
    }

    extent = vk::Extent3D(w, h, 1);
    format = vk::Format::eR8G8B8A8Srgb;

    pixelData.resize(static_cast<long>(w * h) * 4);
    std::memcpy(pixelData.data(), pixels, pixelData.size());

    stbi_image_free(pixels);

    image = Image(device,
                  extent,
                  format,
                  vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
                      vk::ImageUsageFlagBits::eSampled,
                  vk::ImageAspectFlagBits::eColor);

    const vk::SamplerCreateInfo samplerInfo({},
                                            vk::Filter::eLinear,
                                            vk::Filter::eLinear,
                                            vk::SamplerMipmapMode::eLinear,
                                            vk::SamplerAddressMode::eRepeat,
                                            vk::SamplerAddressMode::eRepeat,
                                            vk::SamplerAddressMode::eRepeat,
                                            0.0f,
                                            vk::True,
                                            device.getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy,
                                            vk::False,
                                            vk::CompareOp::eAlways,
                                            0.0f,
                                            vk::LodClampNone,
                                            vk::BorderColor::eIntOpaqueBlack,
                                            vk::False);

    sampler = vk::raii::Sampler(device.get(), samplerInfo);
}

auto Vulkan::Texture::getImage() -> Vulkan::Image& { return image; }
auto Vulkan::Texture::getSampler() -> vk::raii::Sampler& { return sampler; }