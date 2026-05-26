#include "object.hpp"
#include <functional>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <utility>

namespace Render {
Object::Object(Node model) : model(std::move(model)) {};

Object::Object(Node model, std::function<void(Object&, float)> updateFunction)
    : model(std::move(model)), onUpdate(std::move(updateFunction)) {};

void Object::tick(float dt) {
    if (onUpdate) {
        onUpdate(*this, dt);
    }
}

auto Node::getModelMatrix() const -> glm::mat4 {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
    m *= glm::mat4_cast(rotation);
    return glm::scale(m, scale);
}

} // namespace Render