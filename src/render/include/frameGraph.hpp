#pragma once

#include "pass.hpp"
#include "pipelineManager.hpp"
#include "renderTarget.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Render {

class FrameGraph {
  private:
    Vulkan::PipelineManager* pipelineManager;

    std::vector<std::unique_ptr<Pass>> passes;
    std::unordered_map<std::string, PassHandle> nameMap;

    std::unordered_map<RenderTargetHandle, std::vector<Pass*>> producers;
    std::unordered_map<RenderTargetHandle, std::vector<Pass*>> consumers;

    std::vector<Pass*> sortedPasses;

  public:
    FrameGraph(Vulkan::PipelineManager& pipelineManager);
    auto addPass(std::unique_ptr<Pass> pass) -> PassHandle;
    void compile();
    void execute(vk::CommandBuffer cmd);
};
} // namespace Render
