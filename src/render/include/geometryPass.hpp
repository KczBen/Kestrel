#pragma once

#include "camera.hpp"
#include "material.hpp"
#include "object.hpp"
#include "pass.hpp"
#include <optional>

namespace Render {
class GeometryPass : public Pass {
  private:
    Camera* camera;
    std::span<const Object> models;
    std::optional<MaterialHandle> optMaterial;

  public:
    GeometryPass(Vulkan::PipelineManager& pipelineManager,
                 const std::vector<RenderTargetAttachment>& inputs,
                 const std::vector<RenderTargetAttachment>& colorAttachments,
                 const RenderTargetAttachment& depthInput,
                 const RenderTargetAttachment& depthOutput,
                 std::optional<MaterialHandle> optMaterial,
                 std::span<const Object> models,
                 Camera* camera);

    void record(vk::CommandBuffer cmd, uint32_t frameIndex) override;
    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override;
};
} // namespace Render