#pragma once

#include "commandPool.hpp"
#include "descriptorPool.hpp"
#include "deviceBuffer.hpp"
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
#include <functional>
#include <tiny_gltf.h>
#include <unordered_map>
#include <vector>

namespace Render {
class ResourceManager {
  private:
    Vulkan::Device* device;
    std::vector<RenderTarget> renderTargets;

    Vulkan::DescriptorPool descriptorPool;

    std::optional<Vulkan::UploadBuffer> stagingBuffer;
    std::optional<Vulkan::DeviceBuffer> vertexBuffer;
    std::optional<Vulkan::DeviceBuffer> indexBuffer;

    std::vector<Vulkan::HostBuffer> uniformBuffers;

    uint32_t iBufferEnd;
    uint32_t vBufferEnd;

    std::vector<Vulkan::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<Vulkan::DescriptorSet> descriptorSets;

    using SamplerHandle = Vulkan::SamplerHandle;
    using SamplerHandleHash = std::hash<Vulkan::SamplerConfig>;
    std::vector<Vulkan::Sampler> samplers;
    std::unordered_map<Vulkan::SamplerConfig, size_t, SamplerHandleHash> samplerIndex;
    std::vector<Vulkan::Texture> textures;
    std::vector<Material> materials;

    std::optional<Vulkan::PipelineManager> pipelineManager;

    Vulkan::CommandPool uploadPool;

    auto getOrCreateSampler(const Vulkan::SamplerConfig& config) -> SamplerHandle;

    auto loadGLTF(const std::string& path) -> std::vector<Object>;
    auto uploadMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) -> MeshHandle;
    auto loadGltfMaterial(const tinygltf::Model& gltfModel, int materialIndex) -> MaterialHandle;

    auto loadTexture(const std::string& path) -> TextureHandle;
    auto createMaterial(const Material::MaterialConfiguration& config, std::span<Vulkan::Texture*> textures)
        -> MaterialHandle;

  public:
    ResourceManager(Vulkan::PhysicalDevice physicalDevice, Vulkan::Device& device);
    auto getPipelineManager() -> Vulkan::PipelineManager&;
    auto getMaterialList() -> std::vector<Material>&;
    void loadScene(Scene& scene);
    auto createRenderTarget(vk::Extent3D extent,
                            vk::Format format,
                            vk::ImageUsageFlags usageFlags,
                            vk::ImageAspectFlags aspectFlags) -> RenderTargetHandle;
};
} // namespace Render
