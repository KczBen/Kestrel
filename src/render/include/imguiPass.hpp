#pragma once

#include "imgui.h"

#include "imgui_impl_vulkan.h"
#include "pass.hpp"

namespace Render {
class ImGuiPass : public Pass {
  public:
    ImGuiPass(Vulkan::PipelineManager& pipelineManager, const RenderTargetAttachment& colorOutput)
        : Pass(pipelineManager, {}, {colorOutput}, {}, {}) {}

    void record(vk::CommandBuffer cmd) override {
        ImGui::Render();

        ImDrawData* drawData = ImGui::GetDrawData();
        if ((drawData == nullptr) || drawData->CmdListsCount == 0) {
            return;
        }

        ImGui_ImplVulkan_RenderDrawData(drawData, static_cast<VkCommandBuffer>(cmd));
    }

    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override { return 0u; }
};
} // namespace Render