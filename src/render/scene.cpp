#include "scene.hpp"
#include <glm/fwd.hpp>
#include <utility>

namespace Render {
Scene::Scene(std::vector<std::string> modelPaths) : modelPaths(std::move(modelPaths)) {
    // just make *a* camera for now
    cameras.emplace_back();

    playerCamera.emplace(*cameras.data());

    // guess also just put a light here for now
    lights.emplace_back(
        Light{.position = glm::vec3(10.0, 10.0, 10.0), .intensity = 10.0, .colour = glm::vec3(255, 127, 127)});
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
} // namespace Render