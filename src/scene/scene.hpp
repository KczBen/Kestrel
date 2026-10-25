#pragma once

#include "gltfAsset.hpp"
#include "util/input.hpp"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Render {
class ResourceManager;
}

namespace Scene {
class Scene {

  private:
    // *THE* ECS
    entt::registry* entityRegistry;

  public:
    Scene(entt::registry& registry);
    auto getRegistry() -> entt::registry&;
    void update(Input inputs, float deltaTime);
    void addObject(AssetHandle object);
    void addLight(glm::vec3 position, glm::quat rotation, glm::vec3 colour, float intensity, float maxRange);

    friend class Render::ResourceManager;
};
} // namespace Scene