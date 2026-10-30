#pragma once

#include "pass.hpp"
#include "renderTarget.hpp"
#include "resourceManager.hpp"
#include "vulkan-wrapper/pipelineManager.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Render {

class FrameGraph {
  private:
    Vulkan::PipelineManager* pipelineManager;

    SlotMap<std::unique_ptr<Pass>> passes;
    std::unordered_map<std::string, PassHandle> nameMap;

    std::unordered_map<RenderTargetHandle, std::vector<Pass*>> producers;
    std::unordered_map<RenderTargetHandle, std::vector<Pass*>> consumers;

    std::vector<Pass*> sortedPasses;

  public:
    FrameGraph(Vulkan::PipelineManager& pipelineManager);
    auto addPass(std::unique_ptr<Pass> pass) -> PassHandle;
    auto compile() -> const std::vector<Pass*>&;
    void execute(vk::CommandBuffer cmd, const RenderResources& res);
};
} // namespace Render
