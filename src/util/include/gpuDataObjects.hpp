#pragma once

#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS

struct CameraUBO {
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

    // UV indices for each texture (0 = TEXCOORD_0, 1 = TEXCOORD_1)
    alignas(4) int32_t baseColorUVIndex;
    alignas(4) int32_t metallicRoughnessUVIndex;
    alignas(4) int32_t normalUVIndex;
    alignas(4) int32_t aoUVIndex;
    alignas(4) int32_t emissiveUVIndex;

    // KHR_texture_transform - 5 mat4 fields, GLSL mat3 reads first 48 bytes of each
    alignas(16) glm::mat4 baseColorTransformMatrix;
    alignas(16) glm::mat4 metallicRoughnessTransformMatrix;
    alignas(16) glm::mat4 normalTransformMatrix;
    alignas(16) glm::mat4 occlusionTransformMatrix;
    alignas(16) glm::mat4 emissiveTransformMatrix;
};

struct PushConstants {
    glm::mat4 model;
    glm::mat4 normalMatrix;
};
