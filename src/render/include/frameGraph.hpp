#pragma once

#include "pass.hpp"
#include "pipelineManager.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Render {

class FrameGraph {
  private:
    Vulkan::PipelineManager* pipelineManager;

    struct PassInfo {
        size_t originalIndex;
        std::vector<RenderTargetAttachment> colorAttachments;
        std::vector<RenderTargetAttachment> inputs;
        RenderTargetAttachment depthOutput;
        RenderTargetAttachment depthInput;
    };

    struct ImageState {
        vk::ImageLayout layout;
        vk::PipelineStageFlags2 stageMask;
        vk::AccessFlags2 accessFlags;
    };

    std::vector<std::unique_ptr<Pass>> passes;
    std::vector<PassInfo> sortedPasses;
    std::vector<size_t> sortedOrder;

  public:
    FrameGraph(Vulkan::PipelineManager& pipelineManager);
    void addPass(std::unique_ptr<Pass> pass);
    auto compile() -> std::vector<size_t>;
    void execute(vk::CommandBuffer cmd, const std::vector<bool>& passEnabled = {});
};
} // namespace Render
