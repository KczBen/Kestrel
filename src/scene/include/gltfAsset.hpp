#pragma once

#include "material.hpp"
#include "resourceHandles.hpp"
#include "vertex.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

struct glTFPrimitive { // NOLINT
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    MaterialHandle material;
    std::optional<Render::MeshHandle> uploadedMesh;
};

struct glTFNode { // NOLINT
    std::string name;

    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;

    std::vector<glTFPrimitive> primitives;
    std::vector<glTFNode> children;
};

struct glTFObject { // NOLINT
    glTFNode root;
};

using AssetHandle = Render::ResourceHandle<glTFObject>;