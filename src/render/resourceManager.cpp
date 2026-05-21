#include "resourceManager.hpp"
#include "descriptorPool.hpp"
#include "device.hpp"
#include "deviceBuffer.hpp"
#include "gpuDataObjects.hpp"
#include "material.hpp"
#include "pipelineManager.hpp"
#include "resourceHandles.hpp"
#include "uploadBuffer.hpp"
#include "uploadContext.hpp"
#include "vertex.hpp"
#include "vulkan/vulkan.hpp"
#include <SDL3/SDL_filesystem.h>
#include <cstddef>
#include <cstdint>
#include <print>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tiny_gltf.h>

namespace Render {
ResourceManager::ResourceManager(Vulkan::PhysicalDevice physicalDevice, Vulkan::Device& device)
    : device(&device), uploadPool(physicalDevice, device, 2), descriptorPool(device, 4), vBufferEnd(0), iBufferEnd(0) {
    vertexBuffer = Vulkan::DeviceBuffer(
        device, 65565 * sizeof(Vertex), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    indexBuffer = Vulkan::DeviceBuffer(device,
                                       16384 * sizeof(uint32_t),
                                       vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    stagingBuffer = Vulkan::UploadBuffer(device, static_cast<long>(128 * 1024 * 1024));

    uniformBuffers.reserve(16);
    descriptorSetLayouts.reserve(8);
    descriptorSets.reserve(16);
}

auto ResourceManager::loadGLTF(const std::string& path) -> MeshHandle {
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    const bool success = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);
    if (!warn.empty()) {
        std::println("GLTF warning: {}", warn);
    }
    if (!err.empty()) {
        std::println("GLTF error: {}", err);
    }
    if (!success) {
        throw std::runtime_error("Failed to load GLTF: " + path);
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Just load the first mesh/primitive for now
    const auto& mesh = gltfModel.meshes[0];
    const auto& primitive = mesh.primitives[0];

    // --- Indices ---
    {
        const auto& accessor = gltfModel.accessors[primitive.indices];
        const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
        const auto& buffer = gltfModel.buffers[bufferView.buffer];
        const uint8_t* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

        indices.resize(accessor.count);
        switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            for (size_t i = 0; i < accessor.count; i++) {
                uint16_t val = 0;
                std::memcpy(&val, data + (i * sizeof(uint16_t)), sizeof(uint16_t));
                indices[i] = val;
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            for (size_t i = 0; i < accessor.count; i++) {
                uint32_t val = 0;
                std::memcpy(&val, data + (i * sizeof(uint32_t)), sizeof(uint32_t));
                indices[i] = val;
            }
            break;
        default:
            break;
        }
    }

    // --- Positions ---
    auto readAccessor = [&](const std::string& attr) -> std::vector<float> {
        auto it = primitive.attributes.find(attr);
        if (it == primitive.attributes.end()) {
            return {};
        }
        const auto& accessor = gltfModel.accessors[it->second];
        const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
        const auto& buffer = gltfModel.buffers[bufferView.buffer];
        const uint8_t* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

        // Determine how many components per element (glTF spec)
        const size_t components = (attr == "TEXCOORD_0") ? 2 : 3;

        std::vector<float> result(accessor.count * components);
        std::memcpy(result.data(), data, result.size() * sizeof(float));
        return result;
    };

    const std::vector<float> positions = readAccessor("POSITION");
    const std::vector<float> normals = readAccessor("NORMAL");
    const std::vector<float> texCoords = readAccessor("TEXCOORD_0");

    const size_t vertexCount = gltfModel.accessors[primitive.attributes.at("POSITION")].count;
    vertices.resize(vertexCount);

    for (size_t i = 0; i < vertexCount; i++) {
        vertices[i].pos = {positions[i * 3], positions[(i * 3) + 1], positions[(i * 3) + 2]};
        vertices[i].normal = !normals.empty() ? glm::vec3(normals[i * 3], normals[(i * 3) + 1], normals[(i * 3) + 2])
                                              : glm::vec3(0, 1, 0);
        vertices[i].texCoord =
            !texCoords.empty() ? glm::vec2(texCoords[i * 2], texCoords[(i * 2) + 1]) : glm::vec2(0, 0);
    }

    return uploadMesh(vertices, indices);
}

auto ResourceManager::uploadMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    -> MeshHandle {
    MeshHandle handle{.vertexBuffer = &vertexBuffer.value(),
                      .indexBuffer = &indexBuffer.value(),
                      .vertexOffset = vBufferEnd,
                      .vertexCount = static_cast<uint32_t>(vertices.size()),
                      .indexOffset = iBufferEnd,
                      .indexCount = static_cast<uint32_t>(indices.size())};

    auto context = Vulkan::UploadContext(*device, uploadPool, *stagingBuffer);

    context.upload(
        vertexBuffer.value(), vertices.data(), vertices.size() * sizeof(Vertex), vBufferEnd * sizeof(Vertex));

    context.upload(
        indexBuffer.value(), indices.data(), indices.size() * sizeof(uint32_t), iBufferEnd * sizeof(uint32_t));

    vBufferEnd += vertices.size();
    iBufferEnd += indices.size();

    return handle;
}

void ResourceManager::loadScene(Scene& scene) {
    // get camera stuff
    for (auto& camera : scene.cameras) {
        descriptorSetLayouts.emplace_back(*device,
                                          std::vector<vk::DescriptorSetLayoutBinding>{vk::DescriptorSetLayoutBinding(
                                              0,
                                              vk::DescriptorType::eUniformBuffer,
                                              1,
                                              vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)});

        camera.desecriptorSetLayout = &descriptorSetLayouts.back();

        for (auto i = 0; i < 2; i++) {
            uniformBuffers.emplace_back(*device, sizeof(UniformBufferObject));
            camera.uniformBuffers.emplace_back(&uniformBuffers.back());
        }

        for (uint32_t i = 0; i < 2; i++) {
            descriptorSets.emplace_back(*device, descriptorPool, *camera.desecriptorSetLayout);
            const DescriptorWrite uboWrite{.binding = 0,
                                           .type = vk::DescriptorType::eUniformBuffer,
                                           .bufferInfo = {uniformBuffers[i].get(), 0, sizeof(UniformBufferObject)}};
            descriptorSets[i].write(std::span(&uboWrite, 1));
            camera.descriptorSets.emplace_back(&descriptorSets.back());
        }
    }

    pipelineManager.emplace(Vulkan::PipelineManager(*device, descriptorPool, materials));

    // load models
    uint32_t i = 0;
    for (const auto& path : scene.modelPaths) {
        auto handle = loadGLTF(path);
        // should register the materials HERE
        // TODO: Make this actually load materials appropriate to the model
        materials.emplace_back(ShaderDef{.filePath = "shaders/vert.glsl", .stage = EShLangVertex},
                               ShaderDef{.filePath = "shaders/frag.glsl", .stage = EShLangFragment},
                               textures.data(),
                               nullptr,
                               nullptr,
                               nullptr,
                               nullptr,
                               std::vector<vk::PushConstantRange>{
                                   vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4))});

        auto object = Object(handle);
        object.material = i;

        scene.objects.emplace_back(std::move(object));
        i++;
    }

    // TODO: IMPLEMENT https://github.com/ebruneton/precomputed_atmospheric_scattering
    materials.emplace_back(Material(ShaderDef{.filePath = "shaders/skyVert.glsl", .stage = EShLangVertex},
                                    ShaderDef{.filePath = "shaders/skyFrag.glsl", .stage = EShLangFragment},
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    {},
                                    Material::VertexInputMode::None,
                                    vk::CullModeFlagBits::eNone,
                                    true,
                                    false,
                                    vk::CompareOp::eLessOrEqual));

    for (uint32_t i = 0; i < materials.size(); i++) {
        pipelineManager->registerMaterial(i);
    }
}

auto ResourceManager::getPipelineManager() -> Vulkan::PipelineManager& { return *pipelineManager; }
auto ResourceManager::getMaterialList() -> std::vector<Material>& { return materials; }
} // namespace Render
