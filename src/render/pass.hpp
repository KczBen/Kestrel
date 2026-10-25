#pragma once

#include "material.hpp"
#include "renderTarget.hpp"
#include "resourceHandles.hpp"
#include "resourceManager.hpp"
#include "vulkan-wrapper/pipelineManager.hpp"
#include "vulkan-wrapper/sampler.hpp"

#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Render {

class FrameGraph;

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

    auto operator*() const -> RenderTargetHandle { return handle; };
};

struct PassContext {
    Vulkan::PipelineManager* pipelineManager;
    vk::CommandBuffer cmd;
    const RenderResources* res;
};

class Pass {
  private:
    std::string name;
    std::vector<RenderTargetAttachment> colorAttachments;
    std::vector<RenderTargetAttachment> inputs;
    RenderTargetAttachment depthOutput;
    RenderTargetAttachment depthInput;
    std::vector<vk::Format> attachmentFormats;

    std::vector<std::string> dependencies;

  public:
    Pass(const Pass&) = default;
    Pass(Pass&&) = delete;
    auto operator=(const Pass&) -> Pass& = delete;
    auto operator=(Pass&&) = delete;
    Pass(std::string name,
         const std::vector<RenderTargetAttachment>& inputs,
         const std::vector<RenderTargetAttachment>& colorAttachments,
         const RenderTargetAttachment& depthInput,
         const RenderTargetAttachment& depthOutput,
         const std::vector<std::string>& dependencies)
        : name(std::move(name)), colorAttachments(colorAttachments), inputs(inputs), depthOutput(depthOutput),
          depthInput(depthInput), dependencies(dependencies) {
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
    [[nodiscard]] auto getName() const -> std::string { return name; };

    friend class FrameGraph;
};

using PassHandle = ResourceHandle<std::unique_ptr<Pass>>;
} // namespace Render
