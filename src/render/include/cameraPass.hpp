#pragma once

#include "pass.hpp"
#include "scene.hpp"

namespace Render {
class CameraPass : public Pass {
  private:
    Scene::Scene* scene;
    MaterialHandle materialHandle;

  public:
    CameraPass(const std::string& name,
               const std::vector<RenderTargetAttachment>& inputs,
               const std::vector<RenderTargetAttachment>& colorAttachments,
               const RenderTargetAttachment& depthInput,
               const RenderTargetAttachment& depthOutput,
               const std::vector<std::string>& dependencies,
               MaterialHandle materialHandle,
               Scene::Scene* scene);

    void record(PassContext ctx) override;
    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override;
};
} // namespace Render