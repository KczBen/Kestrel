#pragma once

#include "pass.hpp"
#include "scene.hpp"

namespace Render {
class CameraPass : public Pass {
  private:
    Camera* camera;
    Scene* scene;
    MaterialHandle materialHandle;

  public:
    CameraPass(const std::vector<RenderTargetAttachment>& inputs,
               const std::vector<RenderTargetAttachment>& colorAttachments,
               const RenderTargetAttachment& depthInput,
               const RenderTargetAttachment& depthOutput,
               MaterialHandle materialHandle,
               Scene* scene,
               Camera* camera);

    void record(PassContext ctx) override;
    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override;
};
} // namespace Render