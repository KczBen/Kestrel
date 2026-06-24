#pragma once

#include "descriptorSetLayout.hpp"
#include "device.hpp"
#include "shaderDef.hpp"
#include "shaderModule.hpp"
#include "vulkan/vulkan.hpp"
#include <span>
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
                     std::span<const vk::Format> colorAttachmentFormats,
                     std::span<const Vulkan::DescriptorSetLayout*> layouts,
                     std::span<const vk::PushConstantRange> pushConstants,
                     const ShaderDef& vertexShader,
                     const ShaderDef& fragmentShader,
                     bool depthTest,
                     bool depthWrite,
                     vk::CullModeFlagBits cullMode,
                     vk::CompareOp depthCompareMode,
                     vk::BlendOp colorBlendMode,
                     vk::ColorComponentFlags colorWriteMask,
                     vk::SampleCountFlagBits sampleCount,
                     bool useVertexBinding);

    [[nodiscard]] auto operator*() const -> const vk::raii::Pipeline& { return pipeline; }
    [[nodiscard]] auto operator->() const -> const vk::raii::Pipeline* { return &pipeline; }
    auto get() -> const vk::raii::Pipeline&;
    auto getLayout() -> const vk::raii::PipelineLayout&;
};
} // namespace Vulkan