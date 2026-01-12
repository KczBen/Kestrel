#pragma once

#include "vertex.hpp"
#include <cstdint>
#include <glm/fwd.hpp>

class Model {
  public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    glm::vec3 position{};
    glm::quat orientation{};

    Model(const std::string& path);

    // Delete copy constructor and copy assignment
    Model(const Model&) = delete;
    auto operator=(const Model&) -> Model& = delete;

    Model(Model&&) = default;
    auto operator=(Model&&) -> Model& = default;

    ~Model() = default;

    void update(float deltaTime);

    [[nodiscard]] auto getModelMatrix() const -> glm::mat4;
};
