#define GLM_ENABLE_EXPERIMENTAL
#include "camera.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <gtest/gtest.h>

TEST(CameraTest, DefaultConstruction) {
    Render::Camera cam;
    auto pos = cam.getPosition();
    EXPECT_EQ(pos.x, 0.0f);
    EXPECT_EQ(pos.y, 0.0f);
    EXPECT_EQ(pos.z, 0.0f);
}

TEST(CameraTest, CustomPosition) {
    Render::Camera cam({1.0f, 2.0f, 3.0f});
    auto pos = cam.getPosition();
    EXPECT_EQ(pos.x, 1.0f);
    EXPECT_EQ(pos.y, 2.0f);
    EXPECT_EQ(pos.z, 3.0f);
}

TEST(CameraTest, DefaultOrientationIsIdentity) {
    Render::Camera cam;
    auto orient = cam.getOrientation();
    // Default orientation is identity quaternion
    EXPECT_NEAR(glm::length(orient), 1.0f, 1e-6);
}

TEST(CameraTest, SetPosition) {
    Render::Camera cam;
    cam.setPosition({5.0f, 10.0f, 15.0f});
    auto pos = cam.getPosition();
    EXPECT_EQ(pos.x, 5.0f);
    EXPECT_EQ(pos.y, 10.0f);
    EXPECT_EQ(pos.z, 15.0f);
}

TEST(CameraTest, SetOrientation) {
    Render::Camera cam;
    const glm::quat rot = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    cam.setOrientation(rot);
    auto orient = cam.getOrientation();
    // The quaternion should be normalized
    EXPECT_NEAR(glm::length(orient), 1.0f, 1e-6);
}

TEST(CameraTest, DefaultFovIs45Degrees) {
    const Render::Camera cam;
    EXPECT_NEAR(cam.getFov(), 45.0f, 0.01f);
}

TEST(CameraTest, SetFov) {
    Render::Camera cam;
    cam.setFov(60.0f);
    EXPECT_NEAR(cam.getFov(), 60.0f, 0.01f);
}

TEST(CameraTest, SetAspectRatio) {
    Render::Camera cam;
    cam.setAspectRatio(16.0f / 9.0f);
    EXPECT_NEAR(cam.getAspectRatio(), 16.0f / 9.0f, 0.01f);
}

TEST(CameraTest, ProjectionMatrixUsesFov) {
    Render::Camera cam;
    cam.setFov(60.0f);
    auto proj1 = cam.getProjectionMatrix();

    Render::Camera cam2;
    cam2.setFov(30.0f);
    auto proj2 = cam2.getProjectionMatrix();

    // Different FOV should produce different projection matrices
    EXPECT_NE(proj1, proj2);
}

TEST(CameraTest, UpdateViewMatrix) {
    Render::Camera cam({0.0f, 0.0f, 5.0f});
    cam.updateViewMatrix();

    auto view = cam.getViewMatrix();
    auto pos = cam.getPosition();

    // View matrix should reflect the camera position
    EXPECT_NEAR(view[3][0], 0.0f, 0.01f);
    EXPECT_NEAR(view[3][1], 0.0f, 0.01f);
    EXPECT_NEAR(view[3][2], -5.0f, 0.01f);
}

TEST(CameraTest, ProjectionMatrixFlipsY) {
    const Render::Camera cam;
    auto proj = cam.getProjectionMatrix();
    // Vulkan requires -1 on [1][1]
    EXPECT_LT(proj[1][1], 0.0f);
}
