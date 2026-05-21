#pragma once

#include "commandPool.hpp"
#include "descriptorPool.hpp"
#include "deviceBuffer.hpp"
#include "hostBuffer.hpp"
#include "material.hpp"
#include "physicalDevice.hpp"
#include "pipelineManager.hpp"
#include "resourceHandles.hpp"
#include "scene.hpp"
#include "texture.hpp"
#include "uploadBuffer.hpp"
#include "vertex.hpp"
#include <optional>

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

    std::vector<Vulkan::Texture> textures;
    std::vector<Material> materials;

    std::optional<Vulkan::PipelineManager> pipelineManager;

    Vulkan::CommandPool uploadPool;

    auto loadGLTF(const std::string& path) -> MeshHandle;
    auto uploadMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) -> MeshHandle;

    auto loadTexture(const std::string& path) -> Vulkan::Texture&;
    auto createMaterial(const Material::MaterialConfiguration& config, std::span<Vulkan::Texture*> textures)
        -> Material&;

  public:
    ResourceManager(Vulkan::PhysicalDevice physicalDevice, Vulkan::Device& device);
    auto getPipelineManager() -> Vulkan::PipelineManager&;
    auto getMaterialList() -> std::vector<Material>&;
    void loadScene(Scene& scene);
};
} // namespace Render