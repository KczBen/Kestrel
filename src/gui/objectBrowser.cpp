#include "objectBrowser.hpp"

#include "components.hpp"
#include "imgui.h"

namespace Render {

ObjectBrowser::ObjectBrowser(Scene::Scene& scene, const std::vector<Material>& materials)
    : scene(&scene), materials(&materials), registry(scene.getRegistry()) {}

void ObjectBrowser::update() {
    ImGui::Begin("Object Browser", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    auto objectView = registry.view<Scene::Object, Scene::Node, Scene::Children>();

    for (auto [entity, object, node, children] : objectView.each()) {
        ImGui::PushID(static_cast<int>(entity));

        const bool isExpanded =
            ImGui::TreeNodeEx(node.name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding);
        ImGui::PopID();

        if (isExpanded) {
            ImGui::Indent();

            auto& objectChildren = registry.get<Scene::Children>(entity);
            for (auto childEntity : *objectChildren) {
                if (registry.any_of<Scene::Node>(childEntity)) {
                    renderNode(childEntity);
                }
            }

            ImGui::Unindent();
            ImGui::TreePop();
            ImGui::Separator();
        }
    }

    ImGui::End();
}

void ObjectBrowser::renderNode(entt::entity nodeEntity) {
    ImGui::PushID(static_cast<int>(nodeEntity));

    const bool isExpanded = ImGui::TreeNodeEx(registry.get<Scene::Node>(nodeEntity).name.c_str(),
                                              ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding);
    ImGui::PopID();

    if (isExpanded) {
        ImGui::Indent();

        auto& transform = registry.get<Scene::LocalTransform>(nodeEntity);
        ImGui::Text("Pos: (%.2f, %.2f, %.2f)", transform.position.x, transform.position.y, transform.position.z);
        ImGui::Text("Rot: (%.2f, %.2f, %.2f, %.2f)",
                    transform.rotation.x,
                    transform.rotation.y,
                    transform.rotation.z,
                    transform.rotation.w);
        ImGui::Text("Scale: (%.2f, %.2f, %.2f)", transform.scale.x, transform.scale.y, transform.scale.z);

        auto& nodeChildren = registry.get<Scene::Children>(nodeEntity);
        for (auto childEntity : *nodeChildren) {
            if (registry.any_of<Scene::Primitive>(childEntity)) {
                renderPrimitive(childEntity);
            }
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}

void ObjectBrowser::renderPrimitive(entt::entity primitiveEntity) {
    ImGui::PushID(static_cast<int>(primitiveEntity));

    const bool isExpanded =
        ImGui::TreeNodeEx("Primitive", ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding);
    ImGui::PopID();

    if (isExpanded) {
        ImGui::Indent();

        auto& primitive = registry.get<Scene::Primitive>(primitiveEntity);
        if (primitive.material.isPresent()) {
            const auto& material = *primitive.material;
            if (!material.getName().empty()) {
                ImGui::Text("Material: %s", material.getName().c_str());
            }
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}

} // namespace Render
