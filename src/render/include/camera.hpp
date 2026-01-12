#pragma once

#include "input.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

// #include <iostream>

class Camera {
  private:
    // Camera attributes
    glm::vec3 position;
    glm::vec3 worldUp;
    glm::quat orientation;

    // Used for movement
    glm::vec3 front{};
    glm::vec3 up{};
    glm::vec3 right{};

    float movementSpeed = 5.0f;
    float mouseSensitivity = 0.1f;
    float pitchLimit = 90.0f;

    void processRotation(float mouseDeltaX, float mouseDeltaY, float deltaTime) {
        const float yawDelta = -mouseDeltaX * mouseSensitivity;
        const float pitchDelta = -mouseDeltaY * mouseSensitivity;

        const glm::quat yaw = glm::angleAxis(glm::radians(yawDelta), worldUp);

        // Get right vector from the yaw-applied orientation
        const glm::quat tempOrientation = yaw * orientation;
        const glm::vec3 rightVec = glm::normalize(tempOrientation * glm::vec3(1.0f, 0.0f, 0.0f));

        const glm::quat pitch = glm::angleAxis(glm::radians(pitchDelta), rightVec);

        orientation = pitch * tempOrientation;
        updateVectors();
    }

    void processMovement(MovementDirection movement, float deltaTime) {
        const float velocity = movementSpeed * deltaTime;

        if ((movement & MovementDirection::forward) != MovementDirection::none) {
            position += front * velocity;
        }
        if ((movement & MovementDirection::backward) != MovementDirection::none) {
            position -= front * velocity;
        }
        if ((movement & MovementDirection::left) != MovementDirection::none) {
            position -= right * velocity;
        }
        if ((movement & MovementDirection::right) != MovementDirection::none) {
            position += right * velocity;
        }
    }

    void updateVectors() {
        front = glm::normalize(orientation * glm::vec3(0.0f, 0.0f, -1.0f));
        right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

  public:
    Camera(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f),
           const glm::vec3& worldUp = glm::vec3(0.0f, 1.0f, 0.0f))
        : position(position), worldUp(worldUp), orientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) {
        updateVectors();
    }

    // Main update function
    void update(const Input& input, float deltaTime) {
        processRotation(input.mouseX, input.mouseY, deltaTime);
        processMovement(input.movement, deltaTime);

        // printDebugOrientation();
    }

    auto getViewMatrix() -> glm::mat4 {
        const glm::mat4 rotation = glm::transpose(glm::mat4_cast(orientation));
        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), -position);
        return rotation * translation;
    }

    [[nodiscard]] auto getPosition() const -> glm::vec3 { return position; }
    [[nodiscard]] auto getFront() const -> glm::vec3 { return front; }
    [[nodiscard]] auto getUp() const -> glm::vec3 { return up; }
    [[nodiscard]] auto getRight() const -> glm::vec3 { return right; }
    [[nodiscard]] auto getOrientation() const -> glm::quat { return orientation; }

    // Debug: Print camera orientation relative to world coordinates
    // void printDebugOrientation() const {
    //     std::cout << "\e[1;1H\e[2J";
    //     const glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(orientation));
    //
    //     std::cout << "=== Camera Debug Info ===" << '\n';
    //     std::cout << "Position: (" << position.x << ", " << position.y << ", " << position.z << ")" << '\n';
    //     std::cout << "Euler Angles (Pitch, Yaw, Roll): (" << eulerAngles.x << "°, " << eulerAngles.y << "°, "
    //               << eulerAngles.z << "°)" << '\n';
    //     std::cout << "Front Vector: (" << front.x << ", " << front.y << ", " << front.z << ")" << '\n';
    //     std::cout << "Right Vector: (" << right.x << ", " << right.y << ", " << right.z << ")" << '\n';
    //     std::cout << "Up Vector: (" << up.x << ", " << up.y << ", " << up.z << ")" << '\n';
    //     std::cout << "Quaternion (w, x, y, z): (" << orientation.w << ", " << orientation.x << ", " << orientation.y
    //               << ", " << orientation.z << ")" << '\n';
    //     std::cout << "=========================" << '\n';
    // }

    void setMovementSpeed(float speed) { movementSpeed = speed; }
    void setMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
    void setPosition(const glm::vec3& pos) { position = pos; }
    void setOrientation(const glm::quat& quat) {
        orientation = glm::normalize(quat);
        updateVectors();
    }
};
