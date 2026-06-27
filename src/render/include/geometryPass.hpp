#pragma once

#include "camera.hpp"
#include "material.hpp"
#include "object.hpp"
#include "pass.hpp"
#include "scene.hpp"
#include <optional>

namespace Render {
class GeometryPass : public Pass {
  private:
    Scene* scene;
    Camera* camera;
    std::span<const Object> models;
    std::optional<MaterialHandle> optMaterial;

  public:
    GeometryPass(const std::vector<RenderTargetAttachment>& inputs,
                 const std::vector<RenderTargetAttachment>& colorAttachments,
                 const RenderTargetAttachment& depthInput,
                 const RenderTargetAttachment& depthOutput,
                 std::optional<MaterialHandle> optMaterial,
                 Scene* scene,
                 std::span<const Object> models,
                 Camera* camera);

    void record(PassContext ctx) override;
    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override;
};
} // namespace Render