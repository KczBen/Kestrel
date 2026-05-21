#pragma once

#include "resourceHandles.hpp"
#include <functional>
#include <glm/detail/type_quat.hpp>
#include <glm/fwd.hpp>

class Material;

namespace Render {
class Object {
  public:
    MeshHandle mesh;
    MaterialHandle material;

    glm::vec3 position{};
    glm::quat orientation{};

    std::function<void(Object&, float)> onUpdate;

    // Default constructor, places the model at world origin with no scaling and no update function
    Object(MeshHandle mesh);

    // Fully qualified constructor
    Object(MeshHandle mesh, glm::vec3 position, glm::quat orientation, std::function<void(Object&, float)>);

    void tick(float dt);

    [[nodiscard]] auto getModelMatrix() const -> glm::mat4;
};
} // namespace Render
