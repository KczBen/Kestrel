#define GLM_ENABLE_EXPERIMENTAL
#include "camera.hpp"
#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

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
    glm::quat rot = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    cam.setOrientation(rot);
    auto orient = cam.getOrientation();
    // The quaternion should be normalized
    EXPECT_NEAR(glm::length(orient), 1.0f, 1e-6);
}
