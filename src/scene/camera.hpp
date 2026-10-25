#pragma once

#include "components.hpp"
#include "input.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

namespace Scene {
class Camera {
  public:
    static auto getProjectionMatrix(CameraSettings& settings) -> glm::mat4 {
        glm::mat4 proj =
            glm::perspective(glm::radians(settings.fov), settings.aspectRatio, settings.nearPlane, settings.farPlane);
        proj[1][1] *= -1;
        return proj;
    }

    static auto getViewMatrix(CameraTransform& transform) -> glm::mat4 {
        const glm::mat4 rotation = glm::transpose(glm::mat4_cast(transform.orientation));
        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), -transform.position);
        return rotation * translation;
    }

    static auto
    updateOrbitCamera(CameraTransform& transform, OrbitController& controller, const Input& input, float deltaTime)
        -> std::pair<CameraTransform, OrbitController> {
        bool anyChange = false;

        // --- Scroll: dolly in/out ---
        if (input.mouseScrollY != 0.0f) {
            const float zoomDelta = -input.mouseScrollY * controller.zoomSpeed * (controller.orbitRadius * 0.1f);
            controller.orbitRadius =
                std::clamp(controller.orbitRadius + zoomDelta, controller.minRadius, controller.maxRadius);
            anyChange = true;
        }

        if (input.rightMouseButton) {
            const float dx = input.mouseX * controller.mouseSensitivity;
            const float dy = input.mouseY * controller.mouseSensitivity;

            if (input.shiftHeld) {
                // --- Shift + RMB: pan focal point in the camera's local XY plane ---
                if (std::abs(dx) > 1e-6f || std::abs(dy) > 1e-6f) {
                    // Derive the current right/up axes from the orbit state.
                    const glm::vec3 offset = transform.position - controller.focalPoint;
                    const glm::vec3 forward = glm::normalize(-offset);
                    const glm::vec3 right = glm::normalize(glm::cross(forward, transform.worldUp));
                    const glm::vec3 up = glm::cross(right, forward);

                    // Scale pan speed with distance so it feels consistent at any zoom level.
                    const float panScale = controller.orbitRadius * 0.5f;
                    controller.focalPoint -= right * dx * panScale;
                    controller.focalPoint += up * dy * panScale;
                    anyChange = true;
                }
            } else {
                // --- RMB: orbit around focal point ---
                if (std::abs(dx) > 1e-6f || std::abs(dy) > 1e-6f) {
                    controller.yaw -= dx;
                    controller.pitch += dy; // SDL y-axis is top-down, so + dy raises the camera
                    anyChange = true;
                }
            }
        }

        auto newTransform = transform;
        auto newController = controller;
        if (anyChange) {
            rebuildOrbitCamera(newTransform, newController);
        }

        return std::pair(newTransform, newController);
    }

    static void updateFirstPersonCamera(CameraTransform& transform,
                                        CameraSettings& settings,
                                        FirstPersonController& controller,
                                        const Input& input,
                                        float deltaTime) {
        processRotation(transform, controller, input.mouseX, input.mouseY, deltaTime);
        processMovement(transform, controller, input.movement, deltaTime);

        if (input.mouseScrollY != 0.0f) {
            const float fovDelta = -input.mouseScrollY * controller.zoomSpeed * (settings.fov / controller.maxFov);
            const float currentFov = settings.fov;
            settings.fov = glm::clamp(currentFov + fovDelta, controller.minFov, controller.maxFov);
        }
    }

    static void initOrbitCamera(CameraTransform& transform, OrbitController& controller) {
        glm::vec3 offset = transform.position - controller.focalPoint;
        controller.orbitRadius = glm::length(offset);
        if (controller.orbitRadius < 1e-4f) {
            controller.orbitRadius = 5.0f; // initial radius
            offset = glm::vec3(0.0f, 0.0f, controller.orbitRadius);
        }

        const glm::vec3 dir = glm::normalize(offset);
        controller.pitch = std::asin(dir.y);
        controller.yaw = std::atan2(dir.x, dir.z);

        rebuildOrbitCamera(transform, controller);
    }

  private:
    static void rebuildOrbitCamera(CameraTransform& transform, OrbitController& controller) {
        // Clamp pitch to avoid flipping at the poles (leave a tiny margin).
        constexpr float kPitchLimit = glm::radians(89.9f);
        controller.pitch = std::clamp(controller.pitch, -kPitchLimit, kPitchLimit);

        // Spherical - Cartesian offset from the focal point.
        const glm::vec3 offset{controller.orbitRadius * std::cos(controller.pitch) * std::sin(controller.yaw),
                               controller.orbitRadius * std::sin(controller.pitch),
                               controller.orbitRadius * std::cos(controller.pitch) * std::cos(controller.yaw)};

        transform.position = controller.focalPoint + offset;

        // Point the camera at the focal point: build a look-at rotation as a quaternion.
        const glm::vec3 forward = glm::normalize(-offset); // camera looks toward focal point
        const glm::vec3 right = glm::normalize(glm::cross(forward, transform.worldUp));
        const glm::vec3 up = glm::cross(right, forward);

        // Construct rotation matrix columns and convert to quaternion.
        const glm::mat3 rotMat{right, up, -forward}; // column-major: right, up, -forward
        transform.orientation = glm::quat_cast(rotMat);
    }

    static void processRotation(CameraTransform& transform,
                                FirstPersonController& controller,
                                float mouseDeltaX,
                                float mouseDeltaY,
                                float deltaTime) {
        const float yawDelta = -mouseDeltaX * controller.mouseSensitivity;
        const float pitchDelta = -mouseDeltaY * controller.mouseSensitivity;

        const glm::quat yaw = glm::angleAxis(glm::radians(yawDelta), transform.worldUp);

        // Apply yaw first
        const glm::quat tempOrientation = yaw * transform.orientation;

        // Calculate current pitch from the orientation after yaw
        // We use the front vector to determine the pitch.
        const glm::vec3 tempFront = tempOrientation * glm::vec3(0.0f, 0.0f, -1.0f);
        const float currentPitch = glm::degrees(std::asin(glm::clamp(tempFront.y, -1.0f, 1.0f)));

        // Clamp the pitch
        const float newPitch = glm::clamp(currentPitch + pitchDelta, -89.999f, 89.999f);
        const float actualPitchDelta = newPitch - currentPitch;

        if (std::abs(actualPitchDelta) > 0.0001f) {
            const glm::vec3 rightVec = glm::normalize(tempOrientation * glm::vec3(1.0f, 0.0f, 0.0f));
            const glm::quat pitch = glm::angleAxis(glm::radians(actualPitchDelta), rightVec);
            transform.orientation = pitch * tempOrientation;
        } else {
            transform.orientation = tempOrientation;
        }
    }

    static void processMovement(CameraTransform& transform,
                                FirstPersonController& controller,
                                MovementDirection movement,
                                float deltaTime) {
        const float velocity = controller.movementSpeed * deltaTime;

        auto front = glm::normalize(transform.orientation * glm::vec3(0.0f, 0.0f, -1.0f));
        auto right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

        if ((movement & MovementDirection::forward) != MovementDirection::none) {
            transform.position += front * velocity;
        }
        if ((movement & MovementDirection::backward) != MovementDirection::none) {
            transform.position -= front * velocity;
        }
        if ((movement & MovementDirection::left) != MovementDirection::none) {
            transform.position -= right * velocity;
        }
        if ((movement & MovementDirection::right) != MovementDirection::none) {
            transform.position += right * velocity;
        }
    }
};
} // namespace Scene
