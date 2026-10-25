#pragma once

#include "vulkan/vulkan.hpp"
#include <cstddef>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 uv0;
    glm::vec3 normal;
    glm::vec2 uv1;

    static auto getBindingDescription() -> vk::VertexInputBindingDescription {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static auto getAttributeDescriptions() -> std::array<vk::VertexInputAttributeDescription, 5> {
        return {vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
                vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
                vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv0)),
                vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
                vk::VertexInputAttributeDescription(4, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv1))};
    }

    auto operator==(const Vertex& other) const -> bool {
        return pos == other.pos && color == other.color && uv0 == other.uv0 && normal == other.normal &&
               uv1 == other.uv1;
    }
};

template <> struct std::hash<Vertex> {
    auto operator()(const Vertex& vertex) const noexcept -> size_t {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.uv0) << 1) ^ (hash<glm::vec3>()(vertex.normal) << 1) ^
               (hash<glm::vec2>()(vertex.uv1));
    }
};