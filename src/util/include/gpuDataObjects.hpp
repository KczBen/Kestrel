#pragma once

#include <glm/glm.hpp>
#include <optional>
#define GLM_FORCE_RADIANS

struct CameraData {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 invView;
    alignas(16) glm::mat4 invProj;
};

struct CameraUBO {
    CameraData cameras[16];
    uint32_t cameraCount;
    uint32_t pad[3];

    auto operator[](uint32_t slot) -> CameraData& {
        assert(slot < cameraCount && "slot out of range");
        return cameras[slot]; // NOLINT
    }

    auto operator[](uint32_t slot) const -> const CameraData& {
        assert(slot < cameraCount && "slot out of range");
        return cameras[slot]; // NOLINT
    }

    auto allocate() -> uint32_t {
        assert(cameraCount < 16 && "buffer full");
        return cameraCount++;
    }

    auto free(uint32_t slot) -> std::optional<uint32_t> {
        assert(slot < cameraCount && "slot out of range");
        --cameraCount;
        if (slot == cameraCount) {
            return std::nullopt;
        }
        cameras[slot] = cameras[cameraCount]; // NOLINT
        return cameraCount;
    }
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

struct LightData {
    alignas(16) glm::vec3 position;
    alignas(4) float intensity;
    alignas(16) glm::vec3 colour;
    alignas(4) float maxRange;
};

struct LightUBO {
    LightData lights[16];
    uint32_t lightCount;
    uint32_t pad[3];

    auto operator[](uint32_t slot) -> LightData& {
        assert(slot < lightCount && "slot out of range");
        return lights[slot]; // NOLINT
    }

    auto operator[](uint32_t slot) const -> const LightData& {
        assert(slot < lightCount && "slot out of range");
        return lights[slot]; // NOLINT
    }

    auto allocate() -> uint32_t {
        assert(lightCount < 16 && "buffer full");
        return lightCount++;
    }

    auto free(uint32_t slot) -> std::optional<uint32_t> {
        assert(slot < lightCount && "slot out of range");
        --lightCount;
        if (slot == lightCount) {
            return std::nullopt;
        }
        lights[slot] = lights[lightCount]; // NOLINT
        return lightCount;
    }
};