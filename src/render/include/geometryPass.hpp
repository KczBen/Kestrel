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
    GeometryPass(Vulkan::PipelineManager& pipelineManager, std::span<const Object> models, Camera* camera);

    void record(vk::CommandBuffer cmd, uint32_t frameIndex) override;
};
} // namespace Render