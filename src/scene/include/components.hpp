#pragma once

#include "material.hpp"
#include "resourceHandles.hpp"

#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/fwd.hpp>

// All the ECS components go here for now until I figure out how it actually works
namespace Scene {
struct LocalTransform {
    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 scale;
};

struct Primitive {
    Render::MeshHandle mesh{};
    MaterialHandle material;
};

struct Node {
    // nothing here, this is just a tag now
};

struct Object {
    // objects are just nodes at the top level
};

struct Light {
    glm::vec3 colour;
    float intensity;
};

struct Children {
    std::vector<entt::entity> value;

    auto operator*() const noexcept { return value; }
};

struct Parent {
    entt::entity value{entt::null};

    auto operator*() const noexcept { return value; }
};
} // namespace Scene
