#include "scene.hpp"
#include "cameraController.hpp"
#include "gpuDataObjects.hpp"
#include "orbitCamera.hpp"
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <utility>

namespace Render {
Scene::Scene(std::vector<std::string> modelPaths) : modelPaths(std::move(modelPaths)) {
    // just make *a* camera for now
    cameras.emplace_back();

    playerCamera = std::make_unique<OrbitCamera>(cameras.back());

    // guess also just put a light here for now
    lights.emplace_back(glm::vec3(10.0, 10.0, 10.0), 10.0, glm::vec3(255, 127, 127));
};

auto Scene::getCameras() -> std::vector<Camera>& { return cameras; }
void Scene::update(Input inputs, float deltaTime) {
    playerCamera->update(inputs, deltaTime);
    for (auto& object : objects) {
        object.tick(deltaTime);
    }
}
auto Scene::getObjects() const -> std::span<const Object> { return objects; }
auto Scene::getLights() const -> std::span<const Light> { return lights; }

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

    // Collect and pack all lights into a single UBO, also update light camera UBOs
    if (!lights.empty() && !lightUniformBuffers.empty()) {
        LightUBO lightUBO{};
        lightUBO.lightCount = static_cast<uint32_t>(lights.size());

        for (uint32_t i = 0; i < lights.size(); ++i) {
            lightUBO.lights[i].position = lights[i].position;
            lightUBO.lights[i].intensity = lights[i].intensity;
            lightUBO.lights[i].colour = lights[i].colour;
            lightUBO.lights[i].maxRange = lights[i].maxRange;

            // Update embedded camera UBO for each light
            lights[i].camera.updateViewMatrix();
            lights[i].camera.lastUpdatedIndex = (lights[i].camera.lastUpdatedIndex + 1) % 2;
            {
                CameraUBO ubo{};
                ubo.view = lights[i].camera.viewMatrix;
                ubo.proj = lights[i].camera.getProjectionMatrix();
                ubo.invView = glm::inverse(lights[i].camera.viewMatrix);
                ubo.invProj = glm::inverse(ubo.proj);
                lights[i].camera.uniformBuffers[lights[i].camera.lastUpdatedIndex]->update(&ubo, sizeof(CameraUBO));
            }
        }

        // Clear remaining lights
        for (uint32_t i = lights.size(); i < 16; ++i) {
            lightUBO.lights[i].position = glm::vec3(0.0f);
            lightUBO.lights[i].intensity = 0.0f;
            lightUBO.lights[i].colour = glm::vec3(0.0f);
            lightUBO.lights[i].maxRange = 0.0f;
        }

        lightUBOIndex = (lightUBOIndex + 1) % 2;
        lightUniformBuffers[lightUBOIndex]->update(&lightUBO, sizeof(LightUBO));
    }
}
} // namespace Render