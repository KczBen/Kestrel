#pragma once

#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS

struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 invView;
    alignas(16) glm::mat4 invProj;
};

struct MaterialUBO {
    alignas(16) glm::vec4 baseColorFactor;
    alignas(16) glm::vec4 emissiveFactor; // w unused
    alignas(4) float metallicFactor;
    alignas(4) float roughnessFactor;
    alignas(4) float normalScale;
    alignas(4) float occlusionStrength;
};

struct PushConstants {
    glm::mat4 model;
};