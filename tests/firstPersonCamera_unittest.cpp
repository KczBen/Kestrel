#define GLM_ENABLE_EXPERIMENTAL
#include "camera.hpp"
#include "components.hpp"
#include "input.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <gtest/gtest.h>

TEST(FirstPersonCameraTest, Construction) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{0.0f};
    transform.orientation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    Scene::CameraSettings settings;
    Scene::FirstPersonController controller;

    Input input{};

    Scene::Camera::updateFirstPersonCamera(transform, settings, controller, input, 1.0f);

    EXPECT_TRUE(true);
}

TEST(FirstPersonCameraTest, RotationChangesOrientation) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{0.0f};
    transform.orientation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transform.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    Scene::CameraSettings settings;
    Scene::FirstPersonController controller;

    Input input{};
    input.mouseX = 10.0f;
    input.mouseY = 0.0f;

    Scene::Camera::updateFirstPersonCamera(transform, settings, controller, input, 1.0f);

    auto orientation = transform.orientation;
    EXPECT_FALSE(orientation == glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
}

TEST(FirstPersonCameraTest, MovementChangesPosition) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{0.0f, 0.0f, 0.0f};
    transform.orientation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transform.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    Scene::CameraSettings settings;
    Scene::FirstPersonController controller;
    controller.movementSpeed = 5.0f;

    Input input{};
    input.movement = MovementDirection::forward;

    Scene::Camera::updateFirstPersonCamera(transform, settings, controller, input, 1.0f);

    // Moving forward should change the position
    EXPECT_NE(transform.position, glm::vec3{0.0f});
}

TEST(FirstPersonCameraTest, ScrollChangesFov) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{0.0f};
    transform.orientation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    Scene::CameraSettings settings;
    settings.fov = 45.0f;
    Scene::FirstPersonController controller;

    Input input{};
    input.mouseScrollY = 1.0f;

    Scene::Camera::updateFirstPersonCamera(transform, settings, controller, input, 1.0f);

    auto fov = settings.fov;
    EXPECT_NE(fov, 45.0f);
}

TEST(FirstPersonCameraTest, FovClampedToMax) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{0.0f};
    transform.orientation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    Scene::CameraSettings settings;
    settings.fov = 85.0f;
    Scene::FirstPersonController controller;
    controller.maxFov = 90.0f;

    Input input{};
    input.mouseScrollY = 10.0f;

    Scene::Camera::updateFirstPersonCamera(transform, settings, controller, input, 1.0f);

    auto fov = settings.fov;
    EXPECT_LE(fov, controller.maxFov);
}

TEST(FirstPersonCameraTest, FovClampedToMin) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{0.0f};
    transform.orientation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    Scene::CameraSettings settings;
    settings.fov = 20.0f;
    Scene::FirstPersonController controller;
    controller.minFov = 15.0f;

    Input input{};
    input.mouseScrollY = -10.0f;

    Scene::Camera::updateFirstPersonCamera(transform, settings, controller, input, 1.0f);

    auto fov = settings.fov;
    EXPECT_GE(fov, controller.minFov);
}
