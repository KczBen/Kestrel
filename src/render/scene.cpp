#include "scene.hpp"
#include <glm/fwd.hpp>
#include <utility>

namespace Render {
Scene::Scene(std::vector<std::string> modelPaths) : modelPaths(std::move(modelPaths)) {
    // just make *a* camera for now
    cameras.emplace_back();

    playerCamera.emplace(*cameras.data());
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