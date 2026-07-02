#pragma once

#include "camera.hpp"
#include <glm/glm.hpp>

namespace Render {
class Scene;

class Light {
  private:
    glm::vec3 position;
    float intensity;
    glm::vec3 colour;
    float maxRange;

    Camera camera;

  public:
    Light(glm::vec3 position = glm::vec3(0.0f),
          float intensity = 0.0f,
          glm::vec3 colour = glm::vec3(0.0f),
          float maxRange = 0.0f)
        : position(position), intensity(intensity), colour(colour), maxRange(maxRange), camera(position) {};

    friend class Scene;
    friend class ResourceManager;
};
} // namespace Render