#include "vertex.hpp"
#include <glm/detail/qualifier.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <gtest/gtest.h>

TEST(VertexTest, EqualityOperator) {
    auto vertex1 = Vertex{.pos = glm::vec3{0.0, 0.0, 0.0},
                          .color = glm::vec3{127.0, 127.0, 127.0},
                          .texCoord = glm::vec2{0.0, 0.0},
                          .normal = glm::vec3{0.5, 0.5, 0.5}};

    auto vertex2 = Vertex{.pos = glm::vec3{1.0, 1.0, 1.0},
                          .color = glm::vec3{127.0, 127.0, 127.0},
                          .texCoord = glm::vec2{0.0, 0.0},
                          .normal = glm::vec3{0.5, 0.5, 0.5}};

    EXPECT_NE(vertex1, vertex2);

    auto vertex3 = Vertex{.pos = glm::vec3{0.0, 0.0, 0.0},
                          .color = glm::vec3{127.0, 127.0, 127.0},
                          .texCoord = glm::vec2{0.0, 0.0},
                          .normal = glm::vec3{0.5, 0.5, 0.5}};

    EXPECT_EQ(vertex1, vertex3);
}
