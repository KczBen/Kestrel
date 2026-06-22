#pragma once

#include "camera.hpp"
#include "object.hpp"
#include "pass.hpp"

namespace Render {
class GeometryPass : public Pass {
  private:
    Camera* camera;
    std::span<const Object> models;

  public:
    GeometryPass(Vulkan::PipelineManager& pipelineManager,
                 const std::vector<RenderTargetHandle>& targets,
                 const std::vector<RenderTargetHandle>& inputs,
                 std::span<const Object> models,
                 Camera* camera);

    void record(vk::CommandBuffer cmd, uint32_t frameIndex) override;
    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override;
};
} // namespace Render