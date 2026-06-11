#pragma once

#include "camera.hpp"
#include "input.hpp"

namespace Render {
class CameraController {
  private:
    Camera* camera;

    glm::vec3 front{};
    glm::vec3 up{};
    glm::vec3 right{};

    float movementSpeed = 5.0f;
    float mouseSensitivity = 0.1f;
    float pitchLimit = 90.0f;
    float zoomSpeed = 3.0f;
    float minFov = 15.0f;
    float maxFov = 90.0f;

    void processRotation(float mouseDeltaX, float mouseDeltaY, float deltaTime);
    void processMovement(MovementDirection movement, float deltaTime);
    void updateVectors();

  public:
    CameraController(Camera& camera);

    void update(const Input& input, float deltaTime);
};
} // namespace Render