#include "materialBrowser.hpp"

#include "imgui.h"

namespace Render {

MaterialBrowser::MaterialBrowser(const ResourceManager& resourceManager) : resources(&resourceManager) {}

void MaterialBrowser::update() {
    const auto& materials = resources->getMaterials();

    ImGui::Begin("Material Browser", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    materialSettingsView.resize(materials.size(), false);

    for (size_t i = 0; i < materials.size(); ++i) {
        const auto& mat = materials[i];
        const auto& config = mat.getConfiguration();
        const auto& resources = mat.getResources();

        ImGui::PushID(static_cast<int>(i));

        const char* name = mat.getName().empty() ? "(unnamed)" : mat.getName().c_str();
        const bool isExpanded =
            ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding);

        ImGui::PopID();

        if (isExpanded) {
            ImGui::Indent();

            // Pipeline configuration
            ImGui::Text("Depth Test: %s", config.depthTest ? "True" : "False");
            ImGui::Text("Depth Write: %s", config.depthWrite ? "True" : "False");
            ImGui::Text("Depth Compare: %s", depthCompareNames.at(static_cast<size_t>(config.depthCompare)));
            ImGui::Text("Cull Mode: %s", cullModeNames.at(static_cast<size_t>(config.cullMode)));
            ImGui::Text("Blend Mode: %s", blendModeNames.at(static_cast<size_t>(config.blendMode)));
            ImGui::Text("Vertex Input: %s", vertexInputModeNames.at(static_cast<size_t>(config.vertexInputMode)));

            // Shader file paths
            ImGui::Text("Vertex Shader: %s", config.vertexShader.filePath.c_str());
            ImGui::Text("Fragment Shader: %s", config.fragmentShader.filePath.c_str());

            // Resources
            if (!resources.isEmpty()) {
                auto texLabel = [](const char* label, auto&& handle) -> auto {
                    if (handle.isPresent()) {
                        ImGui::Text("%s: Present", label);
                    } else {
                        ImGui::TextColored(ImColor(0.5f, 0.5f, 0.5f), "%s: None", label);
                    }
                };

                texLabel("Base Color", resources.baseColorTexture);
                texLabel("Metallic/Roughness", resources.metallicRoughnessTexture);
                texLabel("Normal", resources.normalTexture);
                texLabel("Occlusion", resources.occlusionTexture);
                texLabel("Emissive", resources.emissiveTexture);
            }

            ImGui::TreePop();

            ImGui::Unindent();
            ImGui::Separator();
        }
    }

    ImGui::End();
}
} // namespace Render
