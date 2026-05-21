#pragma once

#include "pass.hpp"
#include "resourceHandles.hpp"
namespace Render {
class CameraPass : public Pass {
  private:
    Camera* camera;
    MaterialHandle materialHandle;
    std::vector<Material>* materialList;

  public:
    CameraPass(Vulkan::PipelineManager& pipelineManager,
               std::vector<Material>& materialList,
               MaterialHandle materialHandle,
               Camera* camera);

    void record(vk::CommandBuffer cmd, uint32_t frameIndex) override;
};
} // namespace Render