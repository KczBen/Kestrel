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
    [[nodiscard]] virtual auto supportedMaterialTypes() const -> uint32_t {
        return (1u << static_cast<uint32_t>(Material::BlendMode::Opaque)) |
               (1u << static_cast<uint32_t>(Material::BlendMode::Alpha)) |
               (1u << static_cast<uint32_t>(Material::BlendMode::Premultiplied)) |
               (1u << static_cast<uint32_t>(Material::BlendMode::Additive));
    }
    virtual ~Pass() = default;
};
} // namespace Render