#include "scene.hpp"
#include "cameraController.hpp"
#include "components.hpp"
#include "gpuDataObjects.hpp"
#include "orbitCamera.hpp"
#include <entt/entity/fwd.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

namespace Render {
Scene::Scene() {
    // just make *a* camera for now
    cameras.emplace_back();

    playerCamera = std::make_unique<OrbitCamera>(cameras.back());
};

auto Scene::getRegistry() -> entt::registry& { return entityRegistry; }
auto Scene::getCameras() -> std::vector<Camera>& { return cameras; }
void Scene::update(Input inputs, float deltaTime) { playerCamera->update(inputs, deltaTime); }

void Scene::addLight(glm::vec3 position, glm::quat rotation, glm::vec3 colour, float intensity, float maxRange) {
    entt::entity entity = entityRegistry.create();

    entityRegistry.emplace<::Scene::LocalTransform>(entity, rotation, position, glm::vec3(1.0f));
    entityRegistry.emplace<::Scene::Light>(
        entity, ::Scene::Light{.colour = colour, .intensity = intensity, .maxRange = maxRange});
}

auto Scene::getDescriptorSetLayout() -> Vulkan::DescriptorSetLayout& { return *descriptorSetLayout; }
auto Scene::getDescriptorSet() -> Vulkan::DescriptorSet& { return *descriptorSets[lastUpdatedIndex]; }

auto Scene::flipDescriptorSet() -> void { lastUpdatedIndex = (lastUpdatedIndex + 1) % 2; }

void Scene::updateUniformBuffers() {
    // Update each camera's UBO
    for (auto& camera : cameras) {
        camera.updateViewMatrix();
        camera.lastUpdatedIndex = (camera.lastUpdatedIndex + 1) % 2;

        CameraUBO ubo{};
        ubo.view = camera.viewMatrix;
        ubo.proj = camera.getProjectionMatrix();
        ubo.invView = glm::inverse(camera.viewMatrix);
        ubo.invProj = glm::inverse(ubo.proj);
        camera.uniformBuffers[camera.lastUpdatedIndex]->update(&ubo, sizeof(CameraUBO));
    }

    // Collect and pack all lights into a single UBO
    auto& registry = getRegistry();
    auto view = registry.view<::Scene::Light, ::Scene::LocalTransform>();

    if (!lightUniformBuffers.empty()) {
        LightUBO lightUBO{};
        uint32_t lightCount = 0;

        for (auto entity : view) {
            auto& light = registry.get<::Scene::Light>(entity);
            auto& localTransform = registry.get<::Scene::LocalTransform>(entity);

            lightUBO.lights[lightCount].position = localTransform.position;
            lightUBO.lights[lightCount].intensity = light.intensity;
            lightUBO.lights[lightCount].colour = light.colour;
            lightUBO.lights[lightCount].maxRange = light.maxRange;

            ++lightCount;
        }

        lightUBO.lightCount = lightCount;

        // Clear remaining lights
        for (uint32_t j = lightCount; j < 16; ++j) {
            lightUBO.lights[j].position = glm::vec3(0.0f);
            lightUBO.lights[j].intensity = 0.0f;
            lightUBO.lights[j].colour = glm::vec3(0.0f);
            lightUBO.lights[j].maxRange = 0.0f;
        }

        lightUBOIndex = (lightUBOIndex + 1) % 2;
        lightUniformBuffers[lightUBOIndex]->update(&lightUBO, sizeof(LightUBO));
    }
}
} // namespace Render