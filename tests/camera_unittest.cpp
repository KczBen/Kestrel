#define GLM_ENABLE_EXPERIMENTAL
#include "camera.hpp"
#include "components.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

TEST(CameraTest, DefaultProjectionMatrix) {
    Scene::CameraSettings settings;
    auto proj = Scene::Camera::getProjectionMatrix(settings);

    // Projection matrix should be perspective with -1 on [1][1] for Vulkan
    EXPECT_LT(proj[1][1], 0.0f);
}

TEST(CameraTest, ProjectionMatrixFOV) {
    Scene::CameraSettings settings;
    settings.fov = 60.0f;
    auto proj1 = Scene::Camera::getProjectionMatrix(settings);

    Scene::CameraSettings settings2;
    settings2.fov = 30.0f;
    auto proj2 = Scene::Camera::getProjectionMatrix(settings2);

    EXPECT_NE(proj1, proj2);
}

TEST(CameraTest, ProjectionMatrixNearFarPlanes) {
    Scene::CameraSettings settings;
    settings.nearPlane = 0.1f;
    settings.farPlane = 100.0f;
    auto proj = Scene::Camera::getProjectionMatrix(settings);

    // Verify the projection matrix has the expected structure
    // [2][3] and [3][2] should be non-zero for perspective projection
    EXPECT_NE(proj[2][3], 0.0f);
}

TEST(CameraTest, ViewMatrixIdentity) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    transform.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    auto view = Scene::Camera::getViewMatrix(transform);
    // Identity camera should produce an identity view matrix
    EXPECT_NEAR(view[0][0], 1.0f, 0.01f);
    EXPECT_NEAR(view[1][1], 1.0f, 0.01f);
    EXPECT_NEAR(view[2][2], 1.0f, 0.01f);
}

TEST(CameraTest, ViewMatrixTranslation) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3(0.0f, 0.0f, 5.0f);
    transform.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    auto view = Scene::Camera::getViewMatrix(transform);
    EXPECT_NEAR(view[3][2], -5.0f, 0.01f);
}

TEST(CameraTest, ViewMatrixWithCustomWorldUp) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3(0.0f, 0.0f, 3.0f);
    transform.orientation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transform.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    auto view = Scene::Camera::getViewMatrix(transform);
    // The view matrix should reflect the camera position and orientation
    EXPECT_NE(view, glm::mat4(1.0f));
}

TEST(CameraTest, CustomWorldUp) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{};
    transform.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    transform.worldUp = glm::vec3(1.0f, 0.0f, 0.0f);

    auto view = Scene::Camera::getViewMatrix(transform);
    EXPECT_NEAR(view[3][0], 0.0f, 0.01f);
    EXPECT_NEAR(view[3][1], 0.0f, 0.01f);
    EXPECT_NEAR(view[3][2], 0.0f, 0.01f);
}

TEST(OrbitCameraTest, InitOrbitCameraSetsRadius) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{0.0f, 0.0f, 5.0f};

    Scene::OrbitController controller;
    controller.focalPoint = glm::vec3{0.0f};

    Scene::Camera::initOrbitCamera(transform, controller);

    EXPECT_NEAR(controller.orbitRadius, 5.0f, 0.01f);
}

TEST(OrbitCameraTest, InitOrbitCameraFromOffset) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{5.0f, 3.0f, 8.0f};

    Scene::OrbitController controller;
    controller.focalPoint = glm::vec3{0.0f, 0.0f, 0.0f};

    Scene::Camera::initOrbitCamera(transform, controller);

    float expectedRadius = glm::length(transform.position);
    EXPECT_NEAR(controller.orbitRadius, expectedRadius, 0.01f);
}

TEST(OrbitCameraTest, InitOrbitCameraSmallOffset) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{0.0f, 0.0f, 0.0f};

    Scene::OrbitController controller;
    controller.focalPoint = glm::vec3{0.0f, 0.0f, 0.0f};

    Scene::Camera::initOrbitCamera(transform, controller);

    // Should default to radius of 5 when camera is at focal point
    EXPECT_NEAR(controller.orbitRadius, 5.0f, 0.1f);
}

TEST(OrbitCameraTest, UpdateOrbitCameraScroll) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{0.0f, 0.0f, 5.0f};

    Scene::OrbitController controller;
    controller.focalPoint = glm::vec3{0.0f};
    controller.orbitRadius = 5.0f;

    Scene::Camera::initOrbitCamera(transform, controller);
    float initialRadius = controller.orbitRadius;

    Input input{};
    input.mouseScrollY = -1.0f; // Scroll forward

    Scene::Camera::updateOrbitCamera(transform, controller, input, 0.016f);

    // Radius should have changed
    EXPECT_NE(controller.orbitRadius, initialRadius);
}

TEST(OrbitCameraTest, OrbitRadiusClamped) {
    Scene::CameraTransform transform;
    transform.position = glm::vec3{0.0f, 0.0f, 5.0f};

    Scene::OrbitController controller;
    controller.focalPoint = glm::vec3{0.0f};
    controller.orbitRadius = 5.0f;
    controller.minRadius = 1.0f;
    controller.maxRadius = 10.0f;

    Input input{};
    input.mouseScrollY = -100.0f;

    Scene::Camera::updateOrbitCamera(transform, controller, input, 0.016f);

    EXPECT_LE(controller.orbitRadius, controller.maxRadius);
}
