#pragma once

#include "camera.hpp"
#include "input.hpp"

namespace Render {
class CameraController {
  protected:
    Camera* camera;

    float movementSpeed = 5.0f;
    float mouseSensitivity = 0.1f;
    float pitchLimit = 90.0f;
    float zoomSpeed = 3.0f;
    float minFov = 15.0f;
    float maxFov = 90.0f;

  public:
    explicit CameraController(Camera& camera) : camera(&camera) {}
    virtual ~CameraController() = default;
    virtual void update(const Input& input, float deltaTime) = 0;
};
} // namespace Render
