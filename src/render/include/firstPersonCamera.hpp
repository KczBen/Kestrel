#pragma once

#include "cameraController.hpp"

namespace Render {
class FirstPersonCamera : public CameraController {
  private:
    glm::vec3 front{};
    glm::vec3 up{};
    glm::vec3 right{};

    void processRotation(float mouseDeltaX, float mouseDeltaY, float deltaTime);
    void processMovement(MovementDirection movement, float deltaTime);
    void updateVectors();

  public:
    FirstPersonCamera(Camera& camera);
    void update(const Input& input, float deltaTime) override;
};
} // namespace Render
