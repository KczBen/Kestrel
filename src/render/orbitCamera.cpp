#include "orbitCamera.hpp"
#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

namespace Render {
OrbitCamera::OrbitCamera(Camera& camera, const glm::vec3& focalPoint, float initialRadius)
    : CameraController(camera), focalPoint(focalPoint), orbitRadius(initialRadius) {
    // Derive initial yaw/pitch from whatever position the camera already has,
    // so the orbit snaps naturally onto the existing view rather than jumping.
    glm::vec3 offset = camera.getPosition() - focalPoint;
    orbitRadius = glm::length(offset);
    if (orbitRadius < 1e-4f) {
        orbitRadius = initialRadius;
        offset = glm::vec3(0.0f, 0.0f, orbitRadius);
    }

    const glm::vec3 dir = glm::normalize(offset);
    pitch = std::asin(dir.y);
    yaw = std::atan2(dir.x, dir.z);

    rebuildCamera();
}

void OrbitCamera::rebuildCamera() {
    // Clamp pitch to avoid flipping at the poles (leave a tiny margin).
    constexpr float kPitchLimit = glm::radians(89.9f);
    pitch = std::clamp(pitch, -kPitchLimit, kPitchLimit);

    // Spherical - Cartesian offset from the focal point.
    const glm::vec3 offset{orbitRadius * std::cos(pitch) * std::sin(yaw),
                           orbitRadius * std::sin(pitch),
                           orbitRadius * std::cos(pitch) * std::cos(yaw)};

    camera->setPosition(focalPoint + offset);

    // Point the camera at the focal point: build a look-at rotation as a quaternion.
    const glm::vec3 forward = glm::normalize(-offset); // camera looks toward focal point
    const glm::vec3 right = glm::normalize(glm::cross(forward, camera->worldUp));
    const glm::vec3 up = glm::cross(right, forward);

    // Construct rotation matrix columns and convert to quaternion.
    const glm::mat3 rotMat{right, up, -forward}; // column-major: right, up, -forward
    camera->setOrientation(glm::quat_cast(rotMat));
    camera->updateViewMatrix();
}

void OrbitCamera::update(const Input& input, float deltaTime) {
    bool anyChange = false;

    // --- Scroll: dolly in/out ---
    if (input.mouseScrollY != 0.0f) {
        const float zoomDelta = -input.mouseScrollY * zoomSpeed * (orbitRadius * 0.1f);
        orbitRadius = std::clamp(orbitRadius + zoomDelta, minRadius, maxRadius);
        anyChange = true;
    }

    if (input.rightMouseButton) {
        const float dx = input.mouseX * mouseSensitivity;
        const float dy = input.mouseY * mouseSensitivity;

        if (input.shiftHeld) {
            // --- Shift + RMB: pan focal point in the camera's local XY plane ---
            if (std::abs(dx) > 1e-6f || std::abs(dy) > 1e-6f) {
                // Derive the current right/up axes from the orbit state.
                const glm::vec3 offset = camera->getPosition() - focalPoint;
                const glm::vec3 forward = glm::normalize(-offset);
                const glm::vec3 right = glm::normalize(glm::cross(forward, camera->worldUp));
                const glm::vec3 up = glm::cross(right, forward);

                // Scale pan speed with distance so it feels consistent at any zoom level.
                const float panScale = orbitRadius * 0.5f;
                focalPoint -= right * dx * panScale;
                focalPoint += up * dy * panScale;
                anyChange = true;
            }
        } else {
            // --- RMB: orbit around focal point ---
            if (std::abs(dx) > 1e-6f || std::abs(dy) > 1e-6f) {
                yaw -= dx;
                pitch += dy; // SDL y-axis is top-down, so + dy raises the camera
                anyChange = true;
            }
        }
    }

    if (anyChange) {
        rebuildCamera();
    }
}

void OrbitCamera::setFocalPoint(const glm::vec3& point) {
    focalPoint = point;
    rebuildCamera();
}
} // namespace Render