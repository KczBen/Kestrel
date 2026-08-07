#pragma once

#include "commandPool.hpp"
#include "descriptorPool.hpp"
#include "deviceBuffer.hpp"
#include "gltfAsset.hpp"
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
#include <entt/entt.hpp>
#include <functional>
#include <string>
#include <tiny_gltf.h>
#include <unordered_map>
#include <vector>

namespace Render {
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

    std::vector<Vulkan::HostBuffer> uniformBuffers;

    uint32_t iBufferEnd;
    uint32_t vBufferEnd;

    std::vector<Vulkan::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<Vulkan::DescriptorSet> descriptorSets;

    // Rendering internals
    std::vector<Vulkan::Sampler> samplers;
    std::unordered_map<Vulkan::SamplerConfig, size_t, std::hash<Vulkan::SamplerConfig>> samplerIndex;

    Vulkan::PipelineManager pipelineManager;

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

  public:
    ResourceManager(Vulkan::PhysicalDevice physicalDevice, Vulkan::Device& device, entt::registry& registry);
    auto getPipelineManager() -> Vulkan::PipelineManager&;
    auto getMaterial(const std::string& name) -> MaterialHandle;
    void loadScene(Scene::Scene& scene);
    auto importGLTF(const std::string& path) -> AssetHandle;
    auto createRenderTarget(const std::string& name,
                            vk::Extent3D extent,
                            vk::Format format,
                            vk::ImageUsageFlags usageFlags,
                            vk::ImageAspectFlags aspectFlags,
                            vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1) -> RenderTargetHandle;
    auto getRenderTarget(const std::string& name) -> RenderTargetHandle;
    [[nodiscard]] auto getMaterials() const -> const std::vector<Material>& { return materials; }
};
} // namespace Render
