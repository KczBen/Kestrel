#pragma once

#include "gltfAsset.hpp"
#include "material.hpp"
#include "resourceHandles.hpp"

#include <cstdint>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/fwd.hpp>
#include <string>

// All the ECS components go here for now until I figure out how it actually works
namespace Scene {
struct LocalTransform {
    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 scale;
};

struct Primitive {
    Render::MeshHandle mesh{};
    MaterialHandle material;
};

// This signals the resource manager to upload the primitive to the GPU
struct WantsPrimitive {
    glTFPrimitive* primitive;
};

struct Node {
    std::string name;
};

struct Object {
    // objects are just nodes at the top level
};

struct Light {
    glm::vec3 colour;
    float intensity;
    float maxRange;
};

// Camera stuff

struct ActiveCamera {
    // nothing here, just a tag
};

struct CameraIndex {
    uint32_t index; // tells us where in the UBO this camera is
};

struct DirtyUBOs {
    std::array<uint8_t, 2> dirtyUBOs;
};

struct CameraSettings {
    float fov = 45.0f;
    float aspectRatio = 1280.0f / 720.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
};

struct CameraTransform {
    glm::vec3 position;
    glm::quat orientation;
    glm::vec3 worldUp{0.f, 1.f, 0.f};
};

struct FirstPersonController {
    float movementSpeed = 5.f;
    float mouseSensitivity = 0.1f;
    float zoomSpeed = 3.0f;
    float pitchLimit = 90.f;
    float minFov = 15.0f;
    float maxFov = 90.0f;
};

struct OrbitController {
    glm::vec3 focalPoint{0.f};
    float orbitRadius = 5.f;
    float minRadius = 0.5f;
    float maxRadius = 500.f;
    float minFov = 15.0f;
    float maxFov = 90.0f;
    float mouseSensitivity = 0.001f;
    float zoomSpeed = 3.0f;
    float yaw = 0.f;
    float pitch = 0.f;
};

// Generic relation components
struct Children {
    std::vector<entt::entity> value;

    auto operator*() noexcept -> std::vector<entt::entity>& { return value; }
};

struct Parent {
    entt::entity value{entt::null};

    auto operator*() const noexcept { return value; }
};
} // namespace Scene

struct FrameContext {
    uint32_t index = 0; // current frame-in-flight index
};
