#pragma once

#include "material.hpp"
#include "pipelineManager.hpp"
#include "renderTarget.hpp"
#include "sampler.hpp"
#include "vulkan/vulkan.hpp"
#include <span>
#include <vector>

namespace Render {

struct RenderTargetAttachment {
    RenderTargetHandle handle;
    vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eLoad;
    vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eDontCare;
    SamplerHandle sampler;

    RenderTargetAttachment() = default;

    RenderTargetAttachment(RenderTargetHandle handle,
                           vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eLoad,
                           vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eDontCare,
                           SamplerHandle sampler = SamplerHandle{})
        : handle(handle), loadOp(loadOp), storeOp(storeOp), sampler(sampler) {}

    [[nodiscard]] auto isPresent() const -> bool { return handle.isPresent(); }
};

struct PassContext {
    Vulkan::PipelineManager* pipelineManager;
    vk::CommandBuffer cmd;
};

class Pass {
  private:
    std::vector<RenderTargetAttachment> colorAttachments;
    std::vector<RenderTargetAttachment> inputs;
    RenderTargetAttachment depthOutput;
    RenderTargetAttachment depthInput;
    std::vector<vk::Format> attachmentFormats;

  public:
    Pass(const Pass&) = default;
    Pass(Pass&&) = delete;
    auto operator=(const Pass&) -> Pass& = delete;
    auto operator=(Pass&&) = delete;
    Pass(const std::vector<RenderTargetAttachment>& inputs,
         const std::vector<RenderTargetAttachment>& colorAttachments,
         const RenderTargetAttachment& depthInput,
         const RenderTargetAttachment& depthOutput)
        : colorAttachments(colorAttachments), inputs(inputs), depthOutput(depthOutput), depthInput(depthInput) {
        for (const auto& attachment : colorAttachments) {
            if (attachment.handle.isPresent()) {
                attachmentFormats.push_back(attachment.handle->getFormat());
            }
        }
    }

    virtual void record(PassContext ctx) = 0;
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
