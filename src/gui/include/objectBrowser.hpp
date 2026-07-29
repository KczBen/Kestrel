#pragma once

#include "material.hpp"
#include "scene.hpp"
#include <entt/entity/entity.hpp>
#include <vector>

namespace Render {

class ObjectBrowser {
  public:
    explicit ObjectBrowser(Scene::Scene& scene, const std::vector<Material>& materials);

    void update();

  private:
    Scene::Scene* scene;
    const std::vector<Material>* materials;
    entt::registry& registry;

    void renderNode(entt::entity nodeEntity);
    void renderPrimitive(entt::entity primitiveEntity);
};

} // namespace Render
