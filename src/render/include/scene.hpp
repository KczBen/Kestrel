#pragma once

#include "camera.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "firstPersonCamera.hpp"
#include "hostBuffer.hpp"
#include "input.hpp"
#include "light.hpp"
#include "object.hpp"
#include <memory>
#include <optional>
#include <span>
#include <string>

namespace Render {
class Scene {

  private:
    // Scene objects
    std::optional<FirstPersonCamera> playerCamera;
    std::vector<Object> objects;
    std::vector<Light> lights;
    std::vector<Camera> cameras;

    std::vector<std::string> modelPaths;
    uint32_t lastUpdatedIndex = 0;

    Vulkan::DescriptorSetLayoutHandle descriptorSetLayout;
    std::vector<Vulkan::DescriptorSetHandle> descriptorSets;

    // Light UBO handles
    // Not part of the individual lights because one UBO stores all the lights
    std::vector<Vulkan::HostBufferHandle> lightUniformBuffers;
    uint32_t lightUBOIndex = 0;

  public:
    Scene(std::vector<std::string> modelPaths);
    void update(Input inputs, float deltaTime);
    void addObject(std::unique_ptr<Object> model);
    void addLight(std::unique_ptr<Light> light);
    void addControlledCamera(FirstPersonCamera cameraController);
    auto getCameras() -> std::vector<Camera>&;
    [[nodiscard]] auto getObjects() const -> std::span<const Object>;
    [[nodiscard]] auto getLights() const -> std::span<const Light>;

    auto getDescriptorSetLayout() -> Vulkan::DescriptorSetLayout&;
    auto getDescriptorSet() -> Vulkan::DescriptorSet&;
    auto flipDescriptorSet() -> void;
    void updateUniformBuffers();

    friend class ResourceManager;
};
} // namespace Render