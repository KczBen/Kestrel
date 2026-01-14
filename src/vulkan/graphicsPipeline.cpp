#include "graphicsPipeline.hpp"
#include "descriptorSetLayout.hpp"
#include "shaderModule.hpp"
#include "swapchain.hpp"
#include "vertex.hpp"
#include <glm/fwd.hpp>
#include <vulkan/vulkan_raii.hpp>

Vulkan::GraphicsPipeline::GraphicsPipeline(Vulkan::Device& device,
                                           Vulkan::Swapchain& swapchain,
                                           std::span<const Vulkan::DescriptorSetLayout*> layouts,
                                           const ShaderDef& vertexShader,
                                           const ShaderDef& fragmentShader,
                                           bool depthTest,
                                           bool depthWrite,
                                           vk::CullModeFlagBits cullMode,
                                           vk::CompareOp depthCompareMode)
    : device(&device.get()), vertexModule(this->device, vertexShader), fragmentModule(this->device, fragmentShader) {
    const vk::PipelineShaderStageCreateInfo vertexShaderStageInfo(
        vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, vertexModule.get(), "main");

    const vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo(
        vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, fragmentModule.get(), "main");

    const vk::PipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};

    const std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    auto dynamicState = vk::PipelineDynamicStateCreateInfo(
        vk::PipelineDynamicStateCreateFlags(), dynamicStates.size(), dynamicStates.data());

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    const vk::PipelineVertexInputStateCreateInfo vInputInfo(
        {}, 1, &bindingDescription, attributeDescriptions.size(), attributeDescriptions.data());

    const vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, vk::False);

    const vk::Viewport viewport(0.0f,
                                0.0f,
                                static_cast<float>(swapchain.getExtent().width),
                                static_cast<float>(swapchain.getExtent().height),
                                0.0f,
                                1.0f);

    const vk::Rect2D scissor({0, 0}, swapchain.getExtent());

    const vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

    const vk::PushConstantRange pushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4));

    const vk::PipelineRasterizationStateCreateInfo rasterizer({},
                                                              vk::False,
                                                              vk::False,
                                                              vk::PolygonMode::eFill,
                                                              cullMode,
                                                              vk::FrontFace::eCounterClockwise,
                                                              vk::False,
                                                              0.0f,
                                                              0.0f,
                                                              0.0f,
                                                              1.0f);

    const vk::PipelineDepthStencilStateCreateInfo depthStencil({},
                                                               static_cast<vk::Bool32>(depthTest),
                                                               static_cast<vk::Bool32>(depthWrite),
                                                               depthCompareMode,
                                                               vk::False,
                                                               vk::False);

    const vk::PipelineMultisampleStateCreateInfo msaa(
        {}, swapchain.getMsaaSampleCount(), vk::False, 1.0f, {}, vk::False, vk::False);

    const vk::PipelineColorBlendAttachmentState colorBlendAttachment(
        vk::True,
        vk::BlendFactor::eSrcAlpha,
        vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA);

    const vk::PipelineColorBlendStateCreateInfo colorBlending(
        {}, vk::False, vk::LogicOp::eCopy, 1, &colorBlendAttachment, {{0.0f, 0.0f, 0.0f, 0.0f}});

    std::vector<vk::DescriptorSetLayout> rawLayouts;
    rawLayouts.reserve(layouts.size());

    for (auto& layout : layouts) {
        rawLayouts.push_back(layout->get());
    }

    const vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, layouts.size(), rawLayouts.data(), 1, &pushConstantRange);

    layout = vk::raii::PipelineLayout(*this->device, pipelineLayoutInfo);

    const vk::PipelineRenderingCreateInfo pipelineRenderingInfo({}, 1, &swapchain.getFormat(), vk::Format::eD32Sfloat);

    const vk::GraphicsPipelineCreateInfo pipelineInfo({},
                                                      2,
                                                      shaderStages,
                                                      &vInputInfo,
                                                      &inputAssembly,
                                                      {},
                                                      &viewportState,
                                                      &rasterizer,
                                                      &msaa,
                                                      &depthStencil,
                                                      &colorBlending,
                                                      &dynamicState,
                                                      *layout,
                                                      {},
                                                      {},
                                                      {},
                                                      -1,
                                                      &pipelineRenderingInfo);

    pipeline = vk::raii::Pipeline(*this->device, nullptr, pipelineInfo);
}

auto Vulkan::GraphicsPipeline::get() -> const vk::raii::Pipeline& { return pipeline; }

auto Vulkan::GraphicsPipeline::getLayout() -> const vk::raii::PipelineLayout& { return layout; }