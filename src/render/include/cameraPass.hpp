#pragma once

#include "pass.hpp"
namespace Render {
class CameraPass : public Pass {
  private:
    Camera* camera;
    MaterialHandle materialHandle;

  public:
    CameraPass(Vulkan::PipelineManager& pipelineManager, MaterialHandle materialHandle, Camera* camera);

    void record(vk::CommandBuffer cmd, uint32_t frameIndex) override;
};
} // namespace Render