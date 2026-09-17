#pragma once

#include "material.hpp"
#include "scene.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <vector>

namespace Render {

class ObjectBrowser {
  public:
    ObjectBrowser(const entt::registry& registry, const std::vector<Material>& materials);

    void update();

  private:
    const std::vector<Material>* materials;
    const entt::registry* registry;

    void renderNode(entt::entity nodeEntity);
    void renderPrimitive(entt::entity primitiveEntity);
};

} // namespace Render
