#pragma once

#include "cameraController.hpp"

namespace Render {
class OrbitCamera : public CameraController {
  private:
    glm::vec3 focalPoint;

    float orbitRadius;
    float minRadius = 0.5f;
    float maxRadius = 500.0f;

    float mouseSensitivity = 0.001f;

    // Euler angles tracking the spherical coordinates around the focal point.
    // Stored separately so we can clamp pitch without quaternion gimbal issues.
    float yaw = 0.0f;   // radians, horizontal orbit
    float pitch = 0.0f; // radians, vertical orbit

    // Rebuilds the camera position and orientation from focalPoint + yaw/pitch/radius.
    void rebuildCamera();

  public:
    explicit OrbitCamera(Camera& camera, const glm::vec3& focalPoint = glm::vec3(0.0f), float initialRadius = 5.0f);

    void update(const Input& input, float deltaTime) override;

    // Convenience — lets other systems re-target the orbit without disrupting radius/angles.
    void setFocalPoint(const glm::vec3& point);
    [[nodiscard]] auto getFocalPoint() const -> glm::vec3 { return focalPoint; }
};
} // namespace Render