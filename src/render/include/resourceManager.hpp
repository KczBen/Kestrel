#pragma once

#include "commandPool.hpp"
#include "descriptorPool.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "deviceBuffer.hpp"
#include "gltfAsset.hpp"
#include "gpuDataObjects.hpp"
#include "hostBuffer.hpp"
#include "material.hpp"
#include "physicalDevice.hpp"
#include "pipelineManager.hpp"
#include "renderTarget.hpp"
#include "resourceHandles.hpp"
#include "sampler.hpp"
#include "scene.hpp"
#include "texture.hpp"
#include "uploadBuffer.hpp"
#include "vertex.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <functional>
#include <string>
#include <tiny_gltf.h>
#include <unordered_map>
#include <vector>

namespace Render {
struct RenderResources {
    entt::registry* entityRegistry{};
    Vulkan::DescriptorSetLayoutHandle sceneLayout;
    Vulkan::DescriptorSetHandle sceneDescriptorSet;

  private:
    std::vector<Vulkan::DescriptorSetHandle> sceneDescriptorSets;
    uint32_t currentFrameIndex = 0;

    friend class ResourceManager;
};

class ResourceManager {
  private:
    // Vulkan internals
    Vulkan::Device* device;
    Vulkan::DescriptorPool descriptorPool;
    Vulkan::CommandPool uploadPool;

    // GPU storage resources
    std::optional<Vulkan::UploadBuffer> stagingBuffer;
    std::optional<Vulkan::DeviceBuffer> vertexBuffer;
    std::optional<Vulkan::DeviceBuffer> indexBuffer;

    std::vector<Vulkan::HostBuffer<CameraUBO>> cameraUniformBuffers;
    std::vector<Vulkan::HostBuffer<LightUBO>> lightUniformBuffers;
    std::vector<Vulkan::HostBuffer<MaterialUBO>> materialUniformBuffers;

    uint32_t iBufferEnd;
    uint32_t vBufferEnd;

    std::vector<Vulkan::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<Vulkan::DescriptorSet> descriptorSets;

    // Rendering internals
    std::vector<Vulkan::Sampler> samplers;
    std::unordered_map<Vulkan::SamplerConfig, size_t, std::hash<Vulkan::SamplerConfig>> samplerIndex;

    Vulkan::PipelineManager pipelineManager;

    CameraUBO cameraUBOCache{};
    LightUBO lightUBOCache{};

    std::unordered_map<uint32_t, entt::entity> cameraIndexMap;

    std::vector<entt::entity> tagRemovalQueue;

    // Assets
    std::vector<Material> materials;
    std::unordered_map<Material::MaterialKey, MaterialHandle> materialConfigMap;
    std::unordered_map<std::string, MaterialHandle> materialNameMap;
    std::vector<Vulkan::Texture> textures;
    std::vector<glTFObject> assets;

    // Render target stuff
    std::vector<RenderTarget> renderTargets;
    std::unordered_map<std::string, RenderTargetHandle> renderTargetNameMap;

    RenderResources publicResources;

    auto getOrCreateSampler(const Vulkan::SamplerConfig& config) -> SamplerHandle;

    static auto extractTextureTransform(const tinygltf::ExtensionMap& extensions) -> Material::TextureTransform;
    auto uploadMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) -> MeshHandle;
    auto loadGltfMaterial(const tinygltf::Model& gltfModel, int materialIndex) -> MaterialHandle;

    auto loadTexture(const std::string& path) -> TextureHandle;
    auto getOrCreateMaterial(const std::string& name,
                             const Material::MaterialConfiguration& config,
                             const Material::MaterialResources& resources) -> MaterialHandle;

    // EnTT callback stuff
    void onWantsPrimitive(entt::registry& registry, entt::entity entity);
    void onCameraUpdated(entt::registry& registry, entt::entity entity);
    void onLightCreated(entt::registry& registry, entt::entity entity);
    void onFrameTick(entt::registry& registry, entt::entity entity);

  public:
    ResourceManager(Vulkan::PhysicalDevice physicalDevice, Vulkan::Device& device, entt::registry& registry);
    auto getPipelineManager() -> Vulkan::PipelineManager&;
    auto getMaterial(const std::string& name) -> MaterialHandle;
    auto importGLTF(const std::string& path) -> AssetHandle;
    auto createRenderTarget(const std::string& name,
                            vk::Extent3D extent,
                            vk::Format format,
                            vk::ImageUsageFlags usageFlags,
                            vk::ImageAspectFlags aspectFlags,
                            vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1) -> RenderTargetHandle;
    auto getRenderTarget(const std::string& name) -> RenderTargetHandle;
    auto getResources() const -> const RenderResources&;
    [[nodiscard]] auto getMaterials() const -> const std::vector<Material>& { return materials; }
    [[nodiscard]] auto getCameraCount() const -> uint32_t { return cameraUBOCache.cameraCount; }

    // EnTT callback methods (also callable directly for testing)
    void onCameraCreated(entt::registry& registry, entt::entity entity);
    void onCameraDeleted(entt::registry& registry, entt::entity entity);
};
} // namespace Render
