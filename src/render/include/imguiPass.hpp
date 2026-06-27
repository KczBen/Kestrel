#pragma once

#include "imgui.h"

#include "imgui_impl_vulkan.h"
#include "pass.hpp"

namespace Render {
class ImGuiPass : public Pass {
  public:
    ImGuiPass(const RenderTargetAttachment& colorOutput) : Pass({}, {colorOutput}, {}, {}) {}

    void record(PassContext ctx) override {
        ImGui::Render();

        ImDrawData* drawData = ImGui::GetDrawData();
        if ((drawData == nullptr) || drawData->CmdListsCount == 0) {
            return;
        }

        ImGui_ImplVulkan_RenderDrawData(drawData, static_cast<VkCommandBuffer>(ctx.cmd));
    }

    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override { return 0u; }
};
} // namespace Render