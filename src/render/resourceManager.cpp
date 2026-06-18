#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "resourceManager.hpp"
#include "descriptorPool.hpp"
#include "device.hpp"
#include "deviceBuffer.hpp"
#include "gpuDataObjects.hpp"
#include "material.hpp"
#include "pipelineManager.hpp"
#include "resourceHandles.hpp"
#include "sampler.hpp"
#include "shaderDef.hpp"
#include "texture.hpp"
#include "uploadBuffer.hpp"
#include "uploadContext.hpp"
#include "vertex.hpp"
#include "vulkan/vulkan.hpp"
#include <SDL3/SDL_filesystem.h>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <print>
#include <utility>

namespace Render {
ResourceManager::ResourceManager(Vulkan::PhysicalDevice physicalDevice, Vulkan::Device& device)
    : device(&device), uploadPool(physicalDevice, device, 2), descriptorPool(device), vBufferEnd(0), iBufferEnd(0) {
    vertexBuffer = Vulkan::DeviceBuffer(device,
                                        5000000 * sizeof(Vertex),
                                        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    indexBuffer = Vulkan::DeviceBuffer(device,
                                       1000000 * sizeof(uint32_t),
                                       vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    stagingBuffer = Vulkan::UploadBuffer(device, static_cast<long>(512 * 1024 * 1024));

    // TODO: IMPLEMENT https://github.com/ebruneton/precomputed_atmospheric_scattering
    materials.emplace_back(Material(ShaderDef{.filePath = "shaders/skyVert.glsl", .stage = EShLangVertex},
                                    ShaderDef{.filePath = "shaders/skyFrag.glsl", .stage = EShLangFragment},
                                    {},
                                    {},
                                    Material::VertexInputMode::None,
                                    vk::CullModeFlagBits::eNone,
                                    true,
                                    false,
                                    vk::CompareOp::eLessOrEqual));

    auto defaultSamplerConfig = Vulkan::SamplerConfig{};
    auto whitePixelHandle = getOrCreateSampler(defaultSamplerConfig);

    auto context = Vulkan::UploadContext(device, uploadPool, *stagingBuffer);

    auto data = std::array<uint8_t, 4>({255, 255, 255, 255});

    textures.emplace_back(device, &data, 1, 1, whitePixelHandle);
    context.upload(textures.back());
}

auto ResourceManager::loadGLTF(const std::string& path) -> std::vector<Object> {
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool success = false;
    if (path.ends_with(".gltf")) {
        success = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path);
    } else {
        success = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);
    }

    if (!warn.empty()) {
        std::println("GLTF warning: {}", warn);
    }
    if (!err.empty()) {
        std::println("GLTF error: {}", err);
    }
    if (!success) {
        throw std::runtime_error("Failed to load GLTF: " + path);
    }

    auto uploadTexture = [&](int texIndex) -> TextureHandle {
        auto context = Vulkan::UploadContext(*device, uploadPool, *stagingBuffer);
        const auto& tex = gltfModel.textures[texIndex];
        const auto& image = gltfModel.images[tex.source];
        auto texSampler = getOrCreateSampler(Vulkan::SamplerConfig{});
        textures.emplace_back(*device, image.image.data(), image.width, image.height, texSampler);
        context.upload(textures.back());
        return TextureHandle{&textures, textures.size() - 1};
    };

    std::vector<TextureHandle> textureHandles;

    textureHandles.reserve(gltfModel.textures.size());
    for (auto i = 0; i < gltfModel.textures.size(); i++) {
        textureHandles.emplace_back(uploadTexture(i));
    }

    std::vector<MaterialHandle> materialHandles;

    materialHandles.reserve(gltfModel.materials.size());
    for (auto& material : gltfModel.materials) {

        const auto& pbr = material.pbrMetallicRoughness;

        auto loadTexture = [&](int texIndex) -> TextureHandle {
            if (texIndex < 0) {
                return TextureHandle{&textures, 0};
            }

            return textureHandles[texIndex];
        };

        auto baseColor = loadTexture(pbr.baseColorTexture.index);
        auto metallicRoughness = loadTexture(pbr.metallicRoughnessTexture.index);
        auto normal = loadTexture(material.normalTexture.index);
        auto occlusion = loadTexture(material.occlusionTexture.index);
        auto emissive = loadTexture(material.emissiveTexture.index);

        Material::BlendMode blendMode = Material::BlendMode::Opaque;
        if (material.alphaMode == "BLEND" || material.alphaMode == "MASK") { // treat cutout as alpha for now
            blendMode = Material::BlendMode::Alpha;
        }

        const vk::CullModeFlagBits cullMode =
            material.doubleSided ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack;

        Material::MaterialResources resources;
        resources.baseColorTexture = baseColor;
        resources.metallicRoughnessTexture = metallicRoughness;
        resources.normalTexture = normal;
        resources.occlusionTexture = occlusion;
        resources.emissiveTexture = emissive;

        resources.baseColorFactor = pbr.baseColorFactor;
        resources.emissiveFactor = material.emissiveFactor;
        resources.metallicFactor = pbr.metallicFactor;
        resources.roughnessFactor = pbr.roughnessFactor;

        auto mat = Material(ShaderDef{.filePath = "shaders/vert.glsl", .stage = EShLangVertex},
                            ShaderDef{.filePath = "shaders/frag.glsl", .stage = EShLangFragment},
                            resources,
                            std::vector<vk::PushConstantRange>{
                                vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4))},
                            Material::VertexInputMode::Mesh,
                            cullMode,
                            true, // depthTest
                            true, // depthWrite
                            vk::CompareOp::eLess,
                            blendMode);

        uniformBuffers.emplace_back(*device, sizeof(UniformBufferObject));
        mat.materialBufferHandle = {&uniformBuffers, uniformBuffers.size() - 1};

        materials.emplace_back(std::move(mat));

        materialHandles.emplace_back(&materials, materials.size() - 1);
    }

    auto loadMesh = [&](tinygltf::Primitive& primitive) -> std::pair<std::vector<Vertex>, std::vector<uint32_t>> {
        // --- Indices ---
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
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

            const size_t components = tinygltf::GetNumComponentsInType(accessor.type);
            std::vector<float> result(accessor.count * components);
            std::memcpy(result.data(), data, result.size() * sizeof(float));
            return result;
        };

        const std::vector<float> positions = readAccessor("POSITION");
        const std::vector<float> normals = readAccessor("NORMAL");
        const std::vector<float> texCoords = readAccessor("TEXCOORD_0");
        const std::vector<float> colors = readAccessor("COLOR_0");

        const size_t vertexCount = gltfModel.accessors[primitive.attributes.at("POSITION")].count;
        vertices.resize(vertexCount);

        for (size_t i = 0; i < vertexCount; i++) {
            vertices[i].pos = {positions[i * 3], positions[(i * 3) + 1], positions[(i * 3) + 2]};
            vertices[i].normal = !normals.empty()
                                     ? glm::vec3(normals[i * 3], normals[(i * 3) + 1], normals[(i * 3) + 2])
                                     : glm::vec3(0, 1, 0);
            vertices[i].texCoord =
                !texCoords.empty() ? glm::vec2(texCoords[i * 2], texCoords[(i * 2) + 1]) : glm::vec2(0, 0);
            const bool hasColors = primitive.attributes.contains("COLOR_0");
            const bool colorIsVec4 =
                hasColors && gltfModel.accessors[primitive.attributes.at("COLOR_0")].type == TINYGLTF_TYPE_VEC4;

            if (hasColors && !colors.empty()) {
                if (colorIsVec4) {
                    vertices[i].color = glm::vec3(colors[i * 4], colors[(i * 4) + 1], colors[(i * 4) + 2]);
                } else {
                    vertices[i].color =
                        glm::vec4(colors[i * 4], colors[(i * 4) + 1], colors[(i * 4) + 2], colors[(i * 4) + 3]);
                }
            } else {
                vertices[i].color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            }
        }

        return {vertices, indices};
    };

    auto buildNode = [&](this auto& self, tinygltf::Node& gltfNode) -> Node {
        auto node = Node();

        if (!gltfNode.matrix.empty()) {
            glm::mat4 matrix;
            for (auto i = 0; i < 16; ++i) {
                matrix[i / 4][i % 4] = static_cast<float>(gltfNode.matrix[i]);
            }

            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(matrix, node.scale, node.rotation, node.position, skew, perspective);
        } else {
            if (!gltfNode.translation.empty()) {
                node.position = {static_cast<float>(gltfNode.translation[0]),
                                 static_cast<float>(gltfNode.translation[1]),
                                 static_cast<float>(gltfNode.translation[2])};
            } else {
                node.position = glm::vec3(0.0f);
            }

            if (!gltfNode.rotation.empty()) {
                node.rotation = glm::quat(static_cast<float>(gltfNode.rotation[3]),
                                          static_cast<float>(gltfNode.rotation[0]),
                                          static_cast<float>(gltfNode.rotation[1]),
                                          static_cast<float>(gltfNode.rotation[2]));
            } else {
                node.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            }

            if (!gltfNode.scale.empty()) {
                node.scale = {static_cast<float>(gltfNode.scale[0]),
                              static_cast<float>(gltfNode.scale[1]),
                              static_cast<float>(gltfNode.scale[2])};
            } else {
                node.scale = glm::vec3(1.0f);
            }
        }

        if (gltfNode.mesh >= 0) {
            auto gltfMesh = gltfModel.meshes[gltfNode.mesh];
            for (auto& gltfPrimitive : gltfMesh.primitives) {
                auto meshData = loadMesh(gltfPrimitive);
                auto meshHandle = uploadMesh(meshData.first, meshData.second);
                auto materialHandle = materialHandles[gltfPrimitive.material];
                node.primitives.emplace_back(meshHandle, materialHandle);
            }
        }

        for (const auto& childID : gltfNode.children) {
            auto& gltfChild = gltfModel.nodes[childID];
            auto child = self(gltfChild);
            node.children.push_back(child);
        }

        return node;
    };

    // Root nodes correspond to our "Objects"
    std::vector<Object> objects;

    for (const auto& nodeIdx : gltfModel.scenes[gltfModel.defaultScene].nodes) {
        auto& rootNode = gltfModel.nodes[nodeIdx];

        objects.emplace_back(buildNode(rootNode));
    }

    return objects;
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

        camera.desecriptorSetLayout = {&descriptorSetLayouts, descriptorSetLayouts.size() - 1};

        for (auto i = 0; i < 2; i++) {
            uniformBuffers.emplace_back(*device, sizeof(UniformBufferObject));
            camera.uniformBuffers.emplace_back(&uniformBuffers, uniformBuffers.size() - 1);
        }

        for (uint32_t i = 0; i < 2; i++) {
            descriptorSets.emplace_back(*device, descriptorPool, *camera.desecriptorSetLayout);
            const DescriptorWrite uboWrite{.binding = 0,
                                           .type = vk::DescriptorType::eUniformBuffer,
                                           .bufferInfo = {uniformBuffers[i].get(), 0, sizeof(UniformBufferObject)}};
            descriptorSets[i].write(std::span(&uboWrite, 1));
            camera.descriptorSets.emplace_back(&descriptorSets, descriptorSets.size() - 1);
        }
    }

    pipelineManager.emplace(Vulkan::PipelineManager(*device, descriptorPool));

    // load models
    for (const auto& path : scene.modelPaths) {
        auto objects = loadGLTF(path);

        for (auto object : objects) {
            scene.objects.emplace_back(std::move(object));
        }
    }

    for (auto& material : materials) {
        pipelineManager->registerMaterial(material);
    }
}

auto ResourceManager::getPipelineManager() -> Vulkan::PipelineManager& { return *pipelineManager; }
auto ResourceManager::getMaterialList() -> std::vector<Material>& { return materials; }

auto ResourceManager::getOrCreateSampler(const Vulkan::SamplerConfig& config) -> SamplerHandle {
    if (auto it = samplerIndex.find(config); it != samplerIndex.end()) {
        return SamplerHandle{&samplers[it->second]};
    }
    samplers.emplace_back(*device, config);
    auto index = samplers.size() - 1;
    samplerIndex[config] = index;
    return SamplerHandle{&samplers[index]};
}
} // namespace Render
