#pragma once

#include "image.hpp"
#include "resourceHandles.hpp"
#include "sampler.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {

class UploadContext;

class Texture {
  private:
    Vulkan::Image image;
    SamplerHandle samplerHandle;

    std::vector<std::byte> pixelData;
    vk::Extent3D extent;
    vk::Format format;

    bool uploaded = false;

    friend class UploadContext;

  public:
    Texture(Vulkan::Device& device,
            const void* data,
            uint32_t width,
            uint32_t height,
            SamplerHandle sampler,
            vk::Format format = vk::Format::eR8G8B8A8Srgb);
    auto getImage() -> Vulkan::Image&;
    auto getSampler() -> const vk::raii::Sampler&;
};
} // namespace Vulkan

using TextureHandle = Render::ResourceHandle<Vulkan::Texture>;