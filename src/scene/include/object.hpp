#pragma once

#include "material.hpp"
#include "resourceHandles.hpp"
#include <functional>
#include <glm/detail/type_quat.hpp>
#include <glm/fwd.hpp>

class Material;

namespace Render {
struct Primitive {
    MeshHandle mesh{};
    MaterialHandle material;
};

struct Node {
    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 scale;

    // Primitives are technically a Mesh member, but we don't care about meshes
    std::vector<Primitive> primitives;
    std::vector<Node> children;

    [[nodiscard]] auto getModelMatrix() const -> glm::mat4;
};

class Object {
  public:
    Node model;

    std::function<void(Object&, float)> onUpdate;

    // Default constructor, places the model at world origin with no scaling and no update function
    Object(Node model);

    // Fully qualified constructor
    Object(Node model, std::function<void(Object&, float)>);

    void tick(float dt);
};
} // namespace Render
