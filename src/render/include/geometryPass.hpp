#pragma once

#include "material.hpp"
#include "pass.hpp"
#include <optional>

namespace Render {
class GeometryPass : public Pass {
  private:
    std::optional<MaterialHandle> optMaterial;

  public:
    GeometryPass(const std::string& name,
                 const std::vector<RenderTargetAttachment>& inputs,
                 const std::vector<RenderTargetAttachment>& colorAttachments,
                 const RenderTargetAttachment& depthInput,
                 const RenderTargetAttachment& depthOutput,
                 const std::vector<std::string>& dependencies,
                 std::optional<MaterialHandle> optMaterial);

    void record(PassContext ctx) override;
    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override;
};
} // namespace Render