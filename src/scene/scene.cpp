#include "scene.hpp"
#include "camera.hpp"
#include "components.hpp"
#include "gpuDataObjects.hpp"
#include "hostBuffer.hpp"
#include <entt/entity/fwd.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Scene {
Scene::Scene() {
    // just make *a* camera for now
    const entt::entity defaultCamera = entityRegistry.create();
    entityRegistry.emplace<CameraSettings>(defaultCamera);
    entityRegistry.emplace<CameraTransform>(defaultCamera, glm::vec3{}, glm::quat{1.0f, 0.0f, 0.0f, 0.0f});
    entityRegistry.emplace<CameraGpuData>(defaultCamera, std::vector<Vulkan::HostBufferHandle>{}, 0);
    entityRegistry.emplace<OrbitController>(defaultCamera);

    // Mark it active
    entityRegistry.emplace<ActiveCamera>(defaultCamera);

    Camera::initOrbitCamera(entityRegistry.get<CameraTransform>(defaultCamera),
                            entityRegistry.get<OrbitController>(defaultCamera));
};

auto Scene::getRegistry() -> entt::registry& { return entityRegistry; }

void Scene::update(Input inputs, float deltaTime) {
    auto view = entityRegistry.view<CameraTransform, OrbitController, ActiveCamera>();

    for (auto [entity, transform, controller] : view.each()) {
        Camera::updateOrbitCamera(transform, controller, inputs, deltaTime);
    }
}

void Scene::addLight(glm::vec3 position, glm::quat rotation, glm::vec3 colour, float intensity, float maxRange) {
    const entt::entity entity = entityRegistry.create();

    entityRegistry.emplace<LocalTransform>(entity, rotation, position, glm::vec3(1.0f));
    entityRegistry.emplace<Light>(entity, Light{.colour = colour, .intensity = intensity, .maxRange = maxRange});
}

auto Scene::getDescriptorSetLayout() -> Vulkan::DescriptorSetLayout& { return *descriptorSetLayout; }
auto Scene::getDescriptorSet() -> Vulkan::DescriptorSet& { return *descriptorSets[lastUpdatedIndex]; }

auto Scene::flipDescriptorSet() -> void { lastUpdatedIndex = (lastUpdatedIndex + 1) % 2; }

void Scene::updateUniformBuffers() {
    // Update each camera's UBO
    {
        auto view = entityRegistry.view<CameraTransform, CameraSettings, CameraGpuData>();

        for (auto [entity, transform, settings, gpuData] : view.each()) {
            gpuData.lastUpdatedIndex = (gpuData.lastUpdatedIndex + 1) % 2;

            CameraUBO ubo{};
            ubo.view = Camera::getViewMatrix(transform);
            ubo.proj = Camera::getProjectionMatrix(settings);
            ubo.invView = glm::inverse(ubo.view);
            ubo.invProj = glm::inverse(ubo.proj);
            gpuData.uniformBuffers[gpuData.lastUpdatedIndex]->update(&ubo, sizeof(CameraUBO));
        }
    }

    // Collect and pack all lights into a single UBO
    {
        auto view = entityRegistry.view<Light, LocalTransform>();

        if (!lightUniformBuffers.empty()) {
            LightUBO lightUBO{};
            uint32_t lightCount = 0;

            for (auto entity : view) {
                auto& light = view.get<Light>(entity);
                auto& localTransform = view.get<LocalTransform>(entity);

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
}
} // namespace Scene