#pragma once

#include "camera.hpp"
#include "cameraController.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "hostBuffer.hpp"
#include "input.hpp"
#include "object.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <span>

namespace Render {
class Scene {

  private:
    // *THE* ECS
    entt::registry entityRegistry;
    // Scene objects
    std::unique_ptr<CameraController> playerCamera;
    std::vector<Camera> cameras;

    uint32_t lastUpdatedIndex = 0;

    Vulkan::DescriptorSetLayoutHandle descriptorSetLayout;
    std::vector<Vulkan::DescriptorSetHandle> descriptorSets;

    // Light UBO handles
    // Not part of the individual lights because one UBO stores all the lights
    std::vector<Vulkan::HostBufferHandle> lightUniformBuffers;
    uint32_t lightUBOIndex = 0;

  public:
    Scene();
    auto getRegistry() -> entt::registry&;
    void update(Input inputs, float deltaTime);
    void addObject(std::unique_ptr<Object> model);
    void addLight(glm::vec3 position, glm::quat rotation, glm::vec3 colour, float intensity, float maxRange);
    void addControlledCamera(CameraController& cameraController);
    auto getCameras() -> std::vector<Camera>&;
    [[nodiscard]] auto getObjects() const -> std::span<const Object>;

    auto getDescriptorSetLayout() -> Vulkan::DescriptorSetLayout&;
    auto getDescriptorSet() -> Vulkan::DescriptorSet&;
    auto flipDescriptorSet() -> void;
    void updateUniformBuffers();

    friend class ResourceManager;
};
} // namespace Render