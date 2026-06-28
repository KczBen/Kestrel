#pragma once

#include "hostBuffer.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <vector>

namespace Render {

class CameraController;
class FirstPersonCamera;
class ResourceManager;

class Camera {
  private:
    glm::vec3 position;
    glm::vec3 worldUp;
    glm::quat orientation;

    glm::mat4 viewMatrix;
    float fov = 45.0f;
    float aspectRatio = 1280.0f / 720.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    std::vector<Vulkan::HostBufferHandle> uniformBuffers;

    int32_t lastUpdatedIndex;

  public:
    Camera(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f),
           const glm::vec3& worldUp = glm::vec3(0.0f, 1.0f, 0.0f));

    [[nodiscard]] auto getViewMatrix() const -> glm::mat4;
    [[nodiscard]] auto getProjectionMatrix() const -> glm::mat4;
    [[nodiscard]] auto getPosition() const -> glm::vec3;
    [[nodiscard]] auto getOrientation() const -> glm::quat;
    [[nodiscard]] auto getFov() const -> float;
    [[nodiscard]] auto getAspectRatio() const -> float;

    void setPosition(const glm::vec3& pos);
    void setOrientation(const glm::quat& quat);
    void setFov(float newFov);
    void setAspectRatio(float aspect);

    void updateViewMatrix();

    friend class CameraController;
    friend class FirstPersonCamera;
    friend class ResourceManager;
    friend class Scene;
};
} // namespace Render
