#pragma once

#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "gltfAsset.hpp"
#include "hostBuffer.hpp"
#include "input.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Render {
class ResourceManager;
}

namespace Scene {
class Scene {

  private:
    // *THE* ECS
    entt::registry* entityRegistry;
    uint32_t lastUpdatedIndex = 0;

    Vulkan::DescriptorSetLayoutHandle descriptorSetLayout;
    std::vector<Vulkan::DescriptorSetHandle> descriptorSets;

    // Light UBO handles
    // Not part of the individual lights because one UBO stores all the lights
    std::vector<Vulkan::HostBufferHandle> lightUniformBuffers;
    uint32_t lightUBOIndex = 0;

  public:
    Scene(entt::registry& registry);
    auto getRegistry() -> entt::registry&;
    void update(Input inputs, float deltaTime);
    void addObject(AssetHandle object);
    void addLight(glm::vec3 position, glm::quat rotation, glm::vec3 colour, float intensity, float maxRange);

    auto getDescriptorSetLayout() -> Vulkan::DescriptorSetLayout&;
    auto getDescriptorSet() -> Vulkan::DescriptorSet&;
    auto flipDescriptorSet() -> void;
    void updateUniformBuffers();

    friend class Render::ResourceManager;
};
} // namespace Scene