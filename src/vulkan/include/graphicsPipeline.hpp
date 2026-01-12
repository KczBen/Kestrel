#pragma once

#include "descriptorSetLayout.hpp"
#include "device.hpp"
#include "shaderDef.hpp"
#include "shaderModule.hpp"
#include "swapchain.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class GraphicsPipeline {
  private:
    const vk::raii::Device* device;
    vk::raii::PipelineLayout layout = nullptr;
    vk::raii::Pipeline pipeline = nullptr;
    Vulkan::ShaderModule vertexModule;
    Vulkan::ShaderModule fragmentModule;

  public:
    GraphicsPipeline(Vulkan::Device& device,
                     Vulkan::Swapchain& swapchain,
                     std::span<const Vulkan::DescriptorSetLayout*> layouts,
                     const ShaderDef& vertexShader,
                     const ShaderDef& fragmentShader);

    auto get() -> const vk::raii::Pipeline&;
    auto getLayout() -> const vk::raii::PipelineLayout&;
};
} // namespace Vulkan