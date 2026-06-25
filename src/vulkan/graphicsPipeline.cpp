#include "graphicsPipeline.hpp"
#include "descriptorSetLayout.hpp"
#include "shaderModule.hpp"
#include "vertex.hpp"
#include "vulkan/vulkan.hpp"
#include <glm/fwd.hpp>
#include <vulkan/vulkan_raii.hpp>

Vulkan::GraphicsPipeline::GraphicsPipeline(Vulkan::Device& device,
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
                                           bool useVertexBinding)
    : device(&*device), vertexModule(this->device, vertexShader), fragmentModule(this->device, fragmentShader) {
    const vk::PipelineShaderStageCreateInfo vertexShaderStageInfo(
        vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, *vertexModule, "main");

    const vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo(
        vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, *fragmentModule, "main");

    const vk::PipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};

    const std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    auto dynamicState = vk::PipelineDynamicStateCreateInfo(
        vk::PipelineDynamicStateCreateFlags(), dynamicStates.size(), dynamicStates.data());

    vk::VertexInputBindingDescription bindingDescription{};
    std::array<vk::VertexInputAttributeDescription, 5> attributeDescriptions{};
    vk::PipelineVertexInputStateCreateInfo vInputInfo{{}, 0};

    if (useVertexBinding) {
        bindingDescription = Vertex::getBindingDescription();
        attributeDescriptions = Vertex::getAttributeDescriptions();
        vInputInfo = {{}, 1, &bindingDescription, attributeDescriptions.size(), attributeDescriptions.data()};
    }

    const vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, vk::False);

    // This is essentially useless, it could be removed with a Vulkan extension
    const vk::PipelineViewportStateCreateInfo viewportState({}, 1, nullptr, 1, nullptr);

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

    const vk::PipelineMultisampleStateCreateInfo msaa({}, sampleCount, vk::False, 1.0f, {}, vk::False, vk::False);

    const vk::PipelineColorBlendAttachmentState defaultAttachment(
        vk::True,
        vk::BlendFactor::eSrcAlpha,
        vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA);

    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments(colorAttachmentFormats.size(),
                                                                             defaultAttachment);

    const vk::PipelineColorBlendStateCreateInfo colorBlending({},
                                                              vk::False,
                                                              vk::LogicOp::eCopy,
                                                              static_cast<uint32_t>(colorBlendAttachments.size()),
                                                              colorBlendAttachments.data(),
                                                              {{0.0f, 0.0f, 0.0f, 0.0f}});

    std::vector<vk::DescriptorSetLayout> rawLayouts;
    rawLayouts.reserve(layouts.size());

    for (auto& layout : layouts) {
        rawLayouts.push_back(**layout);
    }

    const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {}, layouts.size(), rawLayouts.data(), pushConstants.size(), pushConstants.data());

    layout = vk::raii::PipelineLayout(*this->device, pipelineLayoutInfo);

    const vk::PipelineRenderingCreateInfo pipelineRenderingInfo({},
                                                                static_cast<uint32_t>(colorAttachmentFormats.size()),
                                                                colorAttachmentFormats.data(),
                                                                vk::Format::eD32Sfloat);

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