#pragma once

#include "descriptorPool.hpp"
#include "descriptorSetLayout.hpp"
#include "device.hpp"
#include "render/resourceHandles.hpp"
#include "vulkan/vulkan_raii.hpp"

struct DescriptorWrite {
    uint32_t binding;
    vk::DescriptorType type;
    vk::DescriptorBufferInfo bufferInfo;
    vk::DescriptorImageInfo imageInfo;
};

namespace Vulkan {
class DescriptorSet {
  private:
    const vk::raii::Device* device;
    vk::raii::DescriptorSet descriptorSet = nullptr;

  public:
    DescriptorSet(Vulkan::Device& device, Vulkan::DescriptorPool& pool, Vulkan::DescriptorSetLayout& layout);

    void write(std::span<const DescriptorWrite> writes);

    [[nodiscard]] auto operator*() const -> const vk::raii::DescriptorSet& { return descriptorSet; }
    [[nodiscard]] auto operator->() const -> const vk::raii::DescriptorSet* { return &descriptorSet; }
    [[nodiscard]] auto get() const -> const vk::raii::DescriptorSet&;
};

using DescriptorSetHandle = Render::ResourceHandle<DescriptorSet>;
} // namespace Vulkan