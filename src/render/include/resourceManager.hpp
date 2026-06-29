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
#include <string>
#include <tiny_gltf.h>
#include <unordered_map>
#include <vector>

namespace Render {
class ResourceManager {
  private:
    Vulkan::Device* device;

    Vulkan::DescriptorPool descriptorPool;

    std::optional<Vulkan::UploadBuffer> stagingBuffer;
    std::optional<Vulkan::DeviceBuffer> vertexBuffer;
    std::optional<Vulkan::DeviceBuffer> indexBuffer;

    std::vector<Vulkan::HostBuffer> uniformBuffers;

    uint32_t iBufferEnd;
    uint32_t vBufferEnd;

    std::vector<Vulkan::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<Vulkan::DescriptorSet> descriptorSets;

    std::vector<Vulkan::Sampler> samplers;
    std::unordered_map<Vulkan::SamplerConfig, size_t, std::hash<Vulkan::SamplerConfig>> samplerIndex;
    std::vector<Vulkan::Texture> textures;

    std::vector<Material> materials;
    std::unordered_map<std::string, MaterialHandle> materialNameMap;

    std::vector<RenderTarget> renderTargets;
    std::unordered_map<std::string, RenderTargetHandle> renderTargetNameMap;

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
    auto getMaterial(const std::string& name) -> MaterialHandle;
    void loadScene(Scene& scene);
    auto createRenderTarget(vk::Extent3D extent,
                            vk::Format format,
                            vk::ImageUsageFlags usageFlags,
                            vk::ImageAspectFlags aspectFlags,
                            vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1) -> RenderTargetHandle;
};
} // namespace Render
