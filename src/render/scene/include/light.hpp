#pragma once

#include "camera.hpp"
#include "hostBuffer.hpp"
#include <glm/glm.hpp>

namespace Render {
struct Light {
    glm::vec3 position = glm::vec3(0.0f);
    float intensity = 0.0f;
    glm::vec3 colour = glm::vec3(0.0f);
    float maxRange = 0.0f;

    std::vector<Vulkan::HostBufferHandle> uniformBuffers;

    Camera camera = Camera(position);
};
} // namespace Render