#pragma once

#include "material.hpp"
#include "pipelineManager.hpp"
#include "renderTarget.hpp"
#include "vulkan/vulkan.hpp"
#include <span>
#include <vector>

namespace Render {

struct RenderTargetAttachment {
    RenderTargetHandle handle;
    vk::AttachmentLoadOp loadOp;
    vk::AttachmentStoreOp storeOp;

    RenderTargetAttachment() : loadOp(vk::AttachmentLoadOp::eDontCare), storeOp(vk::AttachmentStoreOp::eDontCare) {}

    RenderTargetAttachment(RenderTargetHandle handle, vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp)
        : handle(handle), loadOp(loadOp), storeOp(storeOp) {}

    [[nodiscard]] auto isPresent() const -> bool { return handle.isPresent(); }
};

class Pass {
  private:
    Vulkan::PipelineManager* pipelineManager;
    std::vector<RenderTargetAttachment> colorAttachments;
    std::vector<RenderTargetAttachment> inputs;
    RenderTargetAttachment depthOutput;
    RenderTargetAttachment depthInput;
    std::vector<vk::Format> attachmentFormats;

  protected:
    [[nodiscard]] auto getPipelineManager() const -> Vulkan::PipelineManager* { return pipelineManager; }

  public:
    Pass(const Pass&) = default;
    Pass(Pass&&) = delete;
    auto operator=(const Pass&) -> Pass& = delete;
    auto operator=(Pass&&) = delete;
    Pass(Vulkan::PipelineManager& pipelineManager,
         const std::vector<RenderTargetAttachment>& inputs,
         const std::vector<RenderTargetAttachment>& colorAttachments,
         const RenderTargetAttachment& depthInput,
         const RenderTargetAttachment& depthOutput)
        : pipelineManager(&pipelineManager), colorAttachments(colorAttachments), inputs(inputs),
          depthOutput(depthOutput), depthInput(depthInput) {
        for (const auto& attachment : colorAttachments) {
            if (attachment.handle.isPresent()) {
                attachmentFormats.push_back(attachment.handle->getFormat());
            }
        }
    }

    virtual void record(vk::CommandBuffer cmd, uint32_t frameIndex) = 0;
    [[nodiscard]] virtual auto supportedMaterialTypes() const -> uint32_t {
        return (1u << static_cast<uint32_t>(Material::BlendMode::Opaque)) |
               (1u << static_cast<uint32_t>(Material::BlendMode::Alpha)) |
               (1u << static_cast<uint32_t>(Material::BlendMode::Premultiplied)) |
               (1u << static_cast<uint32_t>(Material::BlendMode::Additive));
    }
    virtual ~Pass() = default;

    [[nodiscard]] auto getColorAttachment(size_t index) const -> const RenderTargetAttachment& {
        return colorAttachments[index];
    }
    [[nodiscard]] auto getColorAttachmentCount() const -> size_t { return colorAttachments.size(); }
    [[nodiscard]] auto getInputs() const -> const std::vector<RenderTargetAttachment>& { return inputs; }
    [[nodiscard]] auto getDepthOutput() const -> const RenderTargetAttachment& { return depthOutput; }
    [[nodiscard]] auto getDepthInput() const -> const RenderTargetAttachment& { return depthInput; }
    [[nodiscard]] auto getColorAttachmentFormats() const -> std::span<const vk::Format> {
        return {attachmentFormats.data(), attachmentFormats.size()};
    }
};
} // namespace Render
