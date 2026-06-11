#define GLM_ENABLE_EXPERIMENTAL
#include "object.hpp"
#include "resourceHandles.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <gtest/gtest.h>

using namespace Render;

TEST(NodeTest, DefaultConstructor) {
    Node node;
    // Default constructed Node has uninitialized members
    // We can verify the members exist and are accessible
    glm::vec3 testPos{0.0f, 0.0f, 0.0f};
    EXPECT_NO_THROW(node.position = testPos);
}

TEST(NodeTest, SetPosition) {
    Node node;
    node.position = {1.0f, 2.0f, 3.0f};
    EXPECT_EQ(node.position.x, 1.0f);
    EXPECT_EQ(node.position.y, 2.0f);
    EXPECT_EQ(node.position.z, 3.0f);
}

TEST(NodeTest, SetScale) {
    Node node;
    node.scale = {2.0f, 2.0f, 2.0f};
    EXPECT_EQ(node.scale.x, 2.0f);
    EXPECT_EQ(node.scale.y, 2.0f);
    EXPECT_EQ(node.scale.z, 2.0f);
}

TEST(NodeTest, SetRotation) {
    Node node;
    node.rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_NEAR(glm::length(node.rotation), 1.0f, 1e-6);
}

TEST(NodeTest, GetModelMatrixIdentity) {
    Node node;
    node.position = {0.0f, 0.0f, 0.0f};
    node.scale = {1.0f, 1.0f, 1.0f};
    node.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    auto matrix = node.getModelMatrix();
    // Default (identity) model matrix should have identity transform
    EXPECT_EQ(matrix[3].x, 0.0f);
    EXPECT_EQ(matrix[3].y, 0.0f);
    EXPECT_EQ(matrix[3].z, 0.0f);
}

TEST(NodeTest, ModelMatrixWithTranslation) {
    Node node;
    node.position = {5.0f, 0.0f, 0.0f};
    node.scale = {1.0f, 1.0f, 1.0f};
    node.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    auto matrix = node.getModelMatrix();
    // With translation at x=5, the last column should have x=5
    EXPECT_EQ(matrix[3].x, 5.0f);
}

TEST(NodeTest, ModelMatrixWithScale) {
    Node node;
    node.position = {0.0f, 0.0f, 0.0f};
    node.scale = {2.0f, 2.0f, 2.0f};
    node.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    auto matrix = node.getModelMatrix();
    // With uniform scale of 2, the diagonal should be 2
    EXPECT_NEAR(matrix[0].x, 2.0f, 1e-6);
    EXPECT_NEAR(matrix[1].y, 2.0f, 1e-6);
    EXPECT_NEAR(matrix[2].z, 2.0f, 1e-6);
}
