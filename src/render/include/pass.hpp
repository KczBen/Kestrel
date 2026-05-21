#pragma once

#include "material.hpp"
#include "pipelineManager.hpp"
#include "vulkan/vulkan.hpp"

namespace Render {
class Pass {
  protected:
    Vulkan::PipelineManager* pipelineManager;

  public:
    Pass(const Pass&) = default;
    Pass(Pass&&) = delete;
    auto operator=(const Pass&) -> Pass& = delete;
    auto operator=(Pass&&) -> Pass& = delete;
    Pass(Vulkan::PipelineManager& pipelineManager) : pipelineManager(&pipelineManager) {}

    virtual void record(vk::CommandBuffer cmd, uint32_t frameIndex) = 0;
    virtual ~Pass() = default;
};
} // namespace Render