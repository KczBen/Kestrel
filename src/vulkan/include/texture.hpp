#pragma once

#include "image.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {

class UploadContext;

class Texture {
  private:
    Vulkan::Image image;
    vk::raii::Sampler sampler = nullptr;

    std::vector<std::byte> pixelData;
    vk::Extent3D extent;
    vk::Format format;

    bool uploaded = false;

    friend class UploadContext;

  public:
    Texture(Vulkan::Device& device, const char* texturePath);
    auto getImage() -> Vulkan::Image&;
    auto getSampler() -> vk::raii::Sampler&;
};
} // namespace Vulkan