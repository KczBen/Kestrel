#pragma once

#include "pass.hpp"
#include <compare>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Render {

struct PassHandle {
    size_t index;

    [[nodiscard]] auto operator==(const PassHandle&) const -> bool = default;
    [[nodiscard]] auto operator<=>(const PassHandle&) const noexcept -> std::strong_ordering = default;
};

class FrameGraph {
  private:
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
    std::vector<std::vector<PassHandle>> explicitDepsPerPass;
    std::vector<PassInfo> sortedPasses;
    std::vector<size_t> sortedOrder;

  public:
    FrameGraph();
    auto addPass(std::unique_ptr<Pass> pass, const std::vector<PassHandle>& explicitDeps = {}) -> PassHandle;
    auto compile() -> std::vector<size_t>;
    void execute(vk::CommandBuffer cmd, const std::vector<bool>& passEnabled = {});
};
} // namespace Render
