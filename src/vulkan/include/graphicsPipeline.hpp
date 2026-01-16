#pragma once

#include "descriptorSetLayout.hpp"
#include "device.hpp"
#include "shaderDef.hpp"
#include "shaderModule.hpp"
#include "vulkan/vulkan.hpp"
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
    // TODO: Use vk::GraphicsPipelineLibrary to make this more modular
    GraphicsPipeline(Vulkan::Device& device,
                     vk::Format renderTargetFormat,
                     std::span<const Vulkan::DescriptorSetLayout*> layouts,
                     std::span<const vk::PushConstantRange> pushConstants,
                     const ShaderDef& vertexShader,
                     const ShaderDef& fragmentShader,
                     bool depthTest = true,
                     bool depthWrite = true,
                     vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eBack,
                     vk::CompareOp depthCompareMode = vk::CompareOp::eLess,
                     bool useVertexBinding = true);

    auto get() -> const vk::raii::Pipeline&;
    auto getLayout() -> const vk::raii::PipelineLayout&;
};
} // namespace Vulkan