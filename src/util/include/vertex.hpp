#pragma once

#include "vulkan/vulkan.hpp"
#include <cstddef>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;

    static auto getBindingDescription() -> vk::VertexInputBindingDescription {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static auto getAttributeDescriptions() -> std::array<vk::VertexInputAttributeDescription, 4> {
        return {vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
                vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
                vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
                vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal))};
    }

    auto operator==(const Vertex& other) const -> bool {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

template <> struct std::hash<Vertex> {
    auto operator()(const Vertex& vertex) const noexcept -> size_t {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1) ^ (hash<glm::vec3>()(vertex.normal));
    }
};