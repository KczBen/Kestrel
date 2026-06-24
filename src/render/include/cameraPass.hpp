#pragma once

#include "pass.hpp"

namespace Render {
class CameraPass : public Pass {
  private:
    Camera* camera;
    MaterialHandle materialHandle{};

  public:
    CameraPass(Vulkan::PipelineManager& pipelineManager,
               const std::vector<RenderTargetAttachment>& inputs,
               const std::vector<RenderTargetAttachment>& colorAttachments,
               const RenderTargetAttachment& depthInput,
               const RenderTargetAttachment& depthOutput,
               MaterialHandle materialHandle,
               Camera* camera);

    void record(vk::CommandBuffer cmd, uint32_t frameIndex) override;
    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override;
};
} // namespace Render