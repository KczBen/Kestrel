#include "camera.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "gpuDataObjects.hpp"
#include <cstdint>

namespace Render {
Camera::Camera(const glm::vec3& position, const glm::vec3& worldUp)
    : position(position), worldUp(worldUp), orientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
      desecriptorSetLayout(nullptr), lastUpdatedIndex(-1) {}

auto Camera::getViewMatrix() const -> glm::mat4 {
    const glm::mat4 rotation = glm::transpose(glm::mat4_cast(orientation));
    const glm::mat4 translation = glm::translate(glm::mat4(1.0f), -position);
    return rotation * translation;
}

auto Camera::getPosition() const -> glm::vec3 { return position; }
auto Camera::getOrientation() const -> glm::quat { return orientation; }
auto Camera::getDescriptorSetLayout() -> Vulkan::DescriptorSetLayout& { return *desecriptorSetLayout; }
auto Camera::getDescriptorSet(uint32_t index) -> Vulkan::DescriptorSet& { return *descriptorSets[index]; }

void Camera::setPosition(const glm::vec3& pos) { position = pos; }
void Camera::setOrientation(const glm::quat& quat) { orientation = glm::normalize(quat); }

void Camera::updateUniformBuffer(uint32_t index, UniformBufferObject& ubo) {
    uniformBuffers[index]->update(&ubo, sizeof(UniformBufferObject));
    lastUpdatedIndex = static_cast<int32_t>(index);
}
} // namespace Render
