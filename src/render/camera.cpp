#include "camera.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "gpuDataObjects.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdint>

namespace Render {
Camera::Camera(const glm::vec3& position, const glm::vec3& worldUp)
    : position(position), worldUp(worldUp), orientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
      viewMatrix(glm::mat4(1.0f)), desecriptorSetLayout(nullptr), lastUpdatedIndex(-1) {}

auto Camera::getViewMatrix() const -> glm::mat4 { return viewMatrix; }

auto Camera::getProjectionMatrix() const -> glm::mat4 {
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    proj[1][1] *= -1;
    return proj;
}

auto Camera::getPosition() const -> glm::vec3 { return position; }
auto Camera::getOrientation() const -> glm::quat { return orientation; }
auto Camera::getFov() const -> float { return fov; }
auto Camera::getAspectRatio() const -> float { return aspectRatio; }
auto Camera::getDescriptorSetLayout() -> Vulkan::DescriptorSetLayout& { return *desecriptorSetLayout; }
auto Camera::getDescriptorSet(uint32_t index) -> Vulkan::DescriptorSet& { return *descriptorSets[index]; }

void Camera::setPosition(const glm::vec3& pos) { position = pos; }
void Camera::setOrientation(const glm::quat& quat) { orientation = glm::normalize(quat); }
void Camera::setFov(float newFov) { fov = newFov; }
void Camera::setAspectRatio(float aspect) { aspectRatio = aspect; }

void Camera::updateViewMatrix() {
    const glm::mat4 rotation = glm::transpose(glm::mat4_cast(orientation));
    const glm::mat4 translation = glm::translate(glm::mat4(1.0f), -position);
    viewMatrix = rotation * translation;
}

void Camera::updateUniformBuffer(uint32_t index) {
    updateViewMatrix();
    UniformBufferObject ubo{};
    ubo.view = viewMatrix;
    ubo.proj = getProjectionMatrix();
    ubo.invView = glm::inverse(viewMatrix);
    ubo.invProj = glm::inverse(ubo.proj);
    uniformBuffers[index]->update(&ubo, sizeof(UniformBufferObject));
    lastUpdatedIndex = static_cast<int32_t>(index);
}
} // namespace Render
