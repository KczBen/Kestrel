#include "vertex.hpp"
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <gtest/gtest.h>

namespace {

auto makeVertex(glm::vec3 pos) -> Vertex {
    return Vertex{
        .pos = pos,
        .color = glm::vec3{127.0f, 127.0f, 127.0f},
        .texCoord = glm::vec2{0.0f, 0.0f},
        .normal = glm::vec3{0.5f, 0.5f, 0.5f},
    };
}

} // namespace

TEST(VertexTest, EqualVerticesCompareEqual) {
    auto v1 = makeVertex({0.0f, 0.0f, 0.0f});
    auto v2 = makeVertex({0.0f, 0.0f, 0.0f});
    EXPECT_EQ(v1, v2);
}

TEST(VertexTest, DifferentPositionsCompareUnequal) {
    auto v1 = makeVertex({0.0f, 0.0f, 0.0f});
    auto v2 = makeVertex({1.0f, 1.0f, 1.0f});
    EXPECT_NE(v1, v2);
}