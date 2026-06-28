#define GLM_ENABLE_EXPERIMENTAL
#include "firstPersonCamera.hpp"
#include "input.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <gtest/gtest.h>

TEST(FirstPersonCameraTest, Construction) {
    Render::Camera cam;
    Render::FirstPersonCamera controller(cam);
    EXPECT_TRUE(true);
}

TEST(FirstPersonCameraTest, RotationChangesOrientation) {
    Render::Camera cam;
    Render::FirstPersonCamera controller(cam);

    Input input{};
    input.mouseX = 10.0f;
    input.mouseY = 0.0f;

    controller.update(input, 1.0f);

    auto orientation = cam.getOrientation();
    EXPECT_FALSE(orientation == glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
}

TEST(FirstPersonCameraTest, MovementChangesPosition) {
    Render::Camera cam({0.0f, 0.0f, 0.0f});
    Render::FirstPersonCamera controller(cam);

    Input input{};
    input.movement = MovementDirection::forward;

    controller.update(input, 1.0f);

    auto pos = cam.getPosition();
    // Default forward direction is -z, so moving forward decreases z
    EXPECT_LT(pos.z, 0.0f);
}

TEST(FirstPersonCameraTest, ScrollChangesFov) {
    Render::Camera cam;
    cam.setFov(45.0f);
    Render::FirstPersonCamera controller(cam);

    Input input{};
    input.mouseScrollY = 1.0f;

    controller.update(input, 1.0f);

    auto fov = cam.getFov();
    EXPECT_NE(fov, 45.0f);
}

TEST(FirstPersonCameraTest, FovClampedToMax) {
    Render::Camera cam;
    cam.setFov(85.0f);
    Render::FirstPersonCamera controller(cam);

    Input input{};
    input.mouseScrollY = 10.0f;

    controller.update(input, 1.0f);

    auto fov = cam.getFov();
    EXPECT_LE(fov, 90.0f);
}

TEST(FirstPersonCameraTest, FovClampedToMin) {
    Render::Camera cam;
    cam.setFov(20.0f);
    Render::FirstPersonCamera controller(cam);

    Input input{};
    input.mouseScrollY = -10.0f;

    controller.update(input, 1.0f);

    auto fov = cam.getFov();
    EXPECT_GE(fov, 15.0f);
}
