#include "scene.hpp"
#include <glm/fwd.hpp>

namespace Render {
Scene::Scene(const char* modelPath) {
    // just make *a* camera for now
    cameras.emplace_back();

    playerCamera.emplace(*cameras.data());

    modelPaths.emplace_back(modelPath);
};

auto Scene::getCamera() -> Camera& { return *cameras.data(); }
void Scene::update(Input inputs, float deltaTime) {
    playerCamera->update(inputs, deltaTime);
    for (auto& object : objects) {
        object.tick(deltaTime);
    }
}
auto Scene::getObjects() const -> std::span<const Object> { return objects; }
} // namespace Render