#pragma once

#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "gpuDataObjects.hpp"
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
class ResourceManager;

class Camera {
  private:
    glm::vec3 position;
    glm::vec3 worldUp;
    glm::quat orientation;

    Vulkan::DescriptorSetLayout* desecriptorSetLayout;
    std::vector<Vulkan::DescriptorSet*> descriptorSets;
    std::vector<Vulkan::HostBuffer*> uniformBuffers;

    int32_t lastUpdatedIndex;

  public:
    Camera(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f),
           const glm::vec3& worldUp = glm::vec3(0.0f, 1.0f, 0.0f));

    [[nodiscard]] auto getViewMatrix() const -> glm::mat4;
    [[nodiscard]] auto getPosition() const -> glm::vec3;
    [[nodiscard]] auto getOrientation() const -> glm::quat;

    auto getDescriptorSetLayout() -> Vulkan::DescriptorSetLayout&;
    auto getDescriptorSet(uint32_t index) -> Vulkan::DescriptorSet&;

    void setPosition(const glm::vec3& pos);
    void setOrientation(const glm::quat& quat);

    void updateUniformBuffer(uint32_t index, UniformBufferObject& ubo);

    friend class CameraController;
    friend class ResourceManager;
};
} // namespace Render
