#pragma once

#include "camera.hpp"
#include "material.hpp"
#include "pass.hpp"
#include "scene.hpp"
#include <optional>

namespace Render {
class GeometryPass : public Pass {
  private:
    Scene::Scene* scene;
    Camera* camera;
    std::optional<MaterialHandle> optMaterial;

  public:
    GeometryPass(const std::string& name,
                 const std::vector<RenderTargetAttachment>& inputs,
                 const std::vector<RenderTargetAttachment>& colorAttachments,
                 const RenderTargetAttachment& depthInput,
                 const RenderTargetAttachment& depthOutput,
                 const std::vector<std::string>& dependencies,
                 std::optional<MaterialHandle> optMaterial,
                 Scene::Scene* scene,
                 Camera* camera);

    void record(PassContext ctx) override;
    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override;
};
} // namespace Render