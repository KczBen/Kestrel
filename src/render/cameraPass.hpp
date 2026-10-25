#pragma once

#include "pass.hpp"

namespace Render {
class CameraPass : public Pass {
  private:
    MaterialHandle materialHandle;

  public:
    CameraPass(const std::string& name,
               const std::vector<RenderTargetAttachment>& inputs,
               const std::vector<RenderTargetAttachment>& colorAttachments,
               const RenderTargetAttachment& depthInput,
               const RenderTargetAttachment& depthOutput,
               const std::vector<std::string>& dependencies,
               MaterialHandle materialHandle);

    void record(PassContext ctx) override;
    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override;
};
} // namespace Render