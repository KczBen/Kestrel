#include "object.hpp"
#include "resourceHandles.hpp"
#include <functional>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <utility>

Render::Object::Object(Render::MeshHandle mesh, MaterialHandle material)
    : mesh(mesh), material(material), position(glm::vec3{0.0f, 0.0f, 0.0f}),
      orientation(glm::quat{1.0, 0.0, 0.0, 0.0}) {};

Render::Object::Object(Render::MeshHandle mesh,
                       MaterialHandle material,
                       glm::vec3 position,
                       glm::quat orientation,
                       std::function<void(Object&, float)> updateFunction)
    : mesh(mesh), material(material), position(position), orientation(orientation),
      onUpdate(std::move(updateFunction)) {};

void Render::Object::tick(float dt) {
    if (onUpdate) {
        onUpdate(*this, dt);
    }
}

auto Render::Object::getModelMatrix() const -> glm::mat4 {
    return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(orientation);
}