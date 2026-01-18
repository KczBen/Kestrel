#pragma once

#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS

struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 invView;
    alignas(16) glm::mat4 invProj;
};

struct PushConstants {
    glm::mat4 model;
};