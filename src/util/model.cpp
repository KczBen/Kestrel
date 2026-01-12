#include "model.hpp"
#include <glm/ext/quaternion_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#define TINYOBJLOADER_IMPLEMENTATION

#include <tiny_obj_loader.h>

#include <cstddef>

Model::Model(const std::string& path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {attrib.vertices[(3 * index.vertex_index) + 0],
                          attrib.vertices[(3 * index.vertex_index) + 1],
                          attrib.vertices[(3 * index.vertex_index) + 2]};

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (!attrib.colors.empty()) {
                const size_t colorIndex = static_cast<long>(3) * index.vertex_index;
                if (colorIndex + 2 < attrib.colors.size()) {
                    vertex.color = {
                        attrib.colors[colorIndex + 0], attrib.colors[colorIndex + 1], attrib.colors[colorIndex + 2]};
                }
            }

            vertex.texCoord = {attrib.texcoords[(2 * index.texcoord_index) + 0],
                               1.0f - attrib.texcoords[(2 * index.texcoord_index) + 1]};

            vertex.normal = {attrib.normals[(3 * index.normal_index) + 0],
                             attrib.normals[(3 * index.normal_index) + 1],
                             attrib.normals[(3 * index.normal_index) + 2]};

            if (!uniqueVertices.contains(vertex)) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    orientation = glm::rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
}

void Model::update(float deltaTime) {
    // orientation = glm::rotate(orientation, deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

auto Model::getModelMatrix() const -> glm::mat4 {
    return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(orientation);
}