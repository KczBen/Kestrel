#include "cameraController.hpp"

Render::CameraController::CameraController(Camera& camera) : camera(&camera) { updateVectors(); }

void Render::CameraController::processRotation(float mouseDeltaX, float mouseDeltaY, float deltaTime) {
    const float yawDelta = -mouseDeltaX * mouseSensitivity;
    const float pitchDelta = -mouseDeltaY * mouseSensitivity;

    const glm::quat yaw = glm::angleAxis(glm::radians(yawDelta), camera->worldUp);

    // this is broken, it should clamp but doesn't
    const glm::quat tempOrientation = yaw * camera->orientation;
    const glm::vec3 rightVec = glm::normalize(tempOrientation * glm::vec3(1.0f, 0.0f, 0.0f));

    const glm::quat pitch = glm::angleAxis(glm::radians(pitchDelta), rightVec);

    camera->orientation = pitch * tempOrientation;
    updateVectors();
}

void Render::CameraController::processMovement(MovementDirection movement, float deltaTime) {
    const float velocity = movementSpeed * deltaTime;

    if ((movement & MovementDirection::forward) != MovementDirection::none) {
        camera->position += front * velocity;
    }
    if ((movement & MovementDirection::backward) != MovementDirection::none) {
        camera->position -= front * velocity;
    }
    if ((movement & MovementDirection::left) != MovementDirection::none) {
        camera->position -= right * velocity;
    }
    if ((movement & MovementDirection::right) != MovementDirection::none) {
        camera->position += right * velocity;
    }
}

void Render::CameraController::update(const Input& input, float deltaTime) {
    processRotation(input.mouseX, input.mouseY, deltaTime);
    processMovement(input.movement, deltaTime);
}

void Render::CameraController::updateVectors() {
    front = glm::normalize(camera->orientation * glm::vec3(0.0f, 0.0f, -1.0f));
    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::vec3(0.0f, 1.0f, 0.0f);
}