#pragma once

#include "material.hpp"
#include "pipelineManager.hpp"
#include "renderTarget.hpp"
#include "vulkan/vulkan.hpp"
#include <vector>

namespace Render {

struct PassInput {
    RenderTargetHandle texture;
    bool depthComparison = false;
};

class Pass {
  private:
    Vulkan::PipelineManager* pipelineManager;
    std::vector<RenderTargetHandle> targets;
    std::vector<PassInput> inputs;

  protected:
    [[nodiscard]] auto getPipelineManager() const -> Vulkan::PipelineManager* { return pipelineManager; }

  public:
    Pass(const Pass&) = default;
    Pass(Pass&&) = delete;
    auto operator=(const Pass&) -> Pass& = delete;
    auto operator=(Pass&&) -> Pass& = delete;
    Pass(Vulkan::PipelineManager& pipelineManager,
         const std::vector<RenderTargetHandle>& targetHandles,
         const std::vector<PassInput>& inputList)
        : pipelineManager(&pipelineManager), targets(targetHandles), inputs(inputList) {}

    virtual void record(vk::CommandBuffer cmd, uint32_t frameIndex) = 0;
    [[nodiscard]] virtual auto supportedMaterialTypes() const -> uint32_t {
        return (1u << static_cast<uint32_t>(Material::BlendMode::Opaque)) |
               (1u << static_cast<uint32_t>(Material::BlendMode::Alpha)) |
               (1u << static_cast<uint32_t>(Material::BlendMode::Premultiplied)) |
               (1u << static_cast<uint32_t>(Material::BlendMode::Additive));
    }
    virtual ~Pass() = default;

    [[nodiscard]] auto getTarget() const -> const RenderTargetHandle& { return targets.front(); }
    [[nodiscard]] auto getTargets() const -> const std::vector<RenderTargetHandle>& { return targets; }
    [[nodiscard]] auto getInputs() const -> const std::vector<PassInput>& { return inputs; }
};
} // namespace Render
