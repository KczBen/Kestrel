#include "renderTarget.hpp"
#include "scene/camera.hpp"
#include "scene/gltfAsset.hpp"
#include "vulkan-wrapper/descriptorSet.hpp"
#include "vulkan-wrapper/descriptorSetLayout.hpp"
#include <entt/entity/fwd.hpp>
#include <glslang/Public/ShaderLang.h>
#include <string>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "material.hpp"
#include "resourceManager.hpp"
#include "scene/components.hpp"
#include "scene/vertex.hpp"
#include "util/gpuDataObjects.hpp"
#include "util/resourceHandles.hpp"
#include "vulkan-wrapper/descriptorPool.hpp"
#include "vulkan-wrapper/device.hpp"
#include "vulkan-wrapper/deviceBuffer.hpp"
#include "vulkan-wrapper/pipelineManager.hpp"
#include "vulkan-wrapper/sampler.hpp"
#include "vulkan-wrapper/texture.hpp"
#include "vulkan-wrapper/uploadBuffer.hpp"
#include "vulkan-wrapper/uploadContext.hpp"

#include <SDL3/SDL_filesystem.h>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <map>
#include <optional>
#include <print>
#include <utility>
#include <vulkan/vulkan.hpp>

namespace Render {
ResourceManager::ResourceManager(Vulkan::PhysicalDevice physicalDevice,
                                 Vulkan::Device& device,
                                 entt::registry& registry)
    : device(&device), uploadPool(physicalDevice, device, 2), descriptorPool(device), vBufferEnd(0), iBufferEnd(0),
      pipelineManager(device, descriptorPool) {
    vertexBuffer = Vulkan::DeviceBuffer(device,
                                        5000000 * sizeof(Vertex),
                                        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    indexBuffer = Vulkan::DeviceBuffer(device,
                                       1000000 * sizeof(uint32_t),
                                       vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    stagingBuffer = Vulkan::UploadBuffer(device, static_cast<long>(512 * 1024 * 1024));

    publicResources.entityRegistry = &registry;

    auto layoutHandle = descriptorSetLayouts.insert(Vulkan::DescriptorSetLayout(
        device,
        std::vector<vk::DescriptorSetLayoutBinding>{
            vk::DescriptorSetLayoutBinding(0,
                                           vk::DescriptorType::eUniformBufferDynamic,
                                           1,
                                           vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment),
            vk::DescriptorSetLayoutBinding(
                1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment)}));

    publicResources.sceneLayout = layoutHandle;

    for (uint32_t i = 0; i < 2; i++) {
        auto descriptorSet = descriptorSets.insert(Vulkan::DescriptorSet(device, descriptorPool, *layoutHandle));

        std::vector<DescriptorWrite> uboWrites;

        // set up camera UBO
        cameraUniformBuffers.emplace_back(device);
        uboWrites.emplace_back(
            DescriptorWrite{.binding = 0,
                            .type = vk::DescriptorType::eUniformBufferDynamic,
                            .bufferInfo = {cameraUniformBuffers.back().get(), 0, sizeof(CameraData)}});

        // set up light UBO
        lightUniformBuffers.emplace_back(device);
        uboWrites.emplace_back(DescriptorWrite{.binding = 1,
                                               .type = vk::DescriptorType::eUniformBuffer,
                                               .bufferInfo = {lightUniformBuffers.back().get(), 0, sizeof(LightUBO)}});

        publicResources.sceneDescriptorSets.emplace_back(descriptorSet);
        descriptorSet->write(uboWrites);
    }

    publicResources.sceneDescriptorSet = publicResources.sceneDescriptorSets[0];

    registry.on_construct<Scene::WantsPrimitive>().connect<&ResourceManager::onWantsPrimitive>(*this);
    registry.on_construct<Scene::CameraTransform>().connect<&ResourceManager::onCameraCreated>(*this);
    registry.on_update<FrameContext>().connect<&ResourceManager::onFrameTick>(*this);

    registry.on_update<Scene::CameraTransform>().connect<&ResourceManager::onCameraUpdated>(*this);
    // Currently unused
    // registry.on_update<Scene::CameraSettings>().connect<&ResourceManager::onCameraUpdated>(*this);

    registry.on_construct<Scene::LightSettings>().connect<&ResourceManager::onLightCreated>(*this);

    registry.on_destroy<Scene::CameraTransform>().connect<&ResourceManager::onCameraDeleted>(*this);

    // TODO: IMPLEMENT https://github.com/ebruneton/precomputed_atmospheric_scattering
    getOrCreateMaterial("sky",
                        {.vertexShader = {.filePath = "shaders/skyVert.glsl", .stage = EShLangVertex},
                         .fragmentShader = {.filePath = "shaders/skyFrag.glsl", .stage = EShLangFragment},
                         .vertexInputMode = Material::VertexInputMode::None,
                         .topology = vk::PrimitiveTopology::eTriangleList,
                         .cullMode = vk::CullModeFlagBits::eNone,
                         .depthTest = true,
                         .depthWrite = false,
                         .depthCompare = vk::CompareOp::eLessOrEqual,
                         .blendMode = Material::BlendMode::Opaque,
                         .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                           vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
                         .descriptorSetBindings = {},
                         .pushConstants = {}},
                        {});

    getOrCreateMaterial(
        "depth_only",
        {.vertexShader = {.filePath = "shaders/depthVert.glsl", .stage = EShLangVertex},
         .fragmentShader = {.filePath = "shaders/depthFrag.glsl", .stage = EShLangFragment},
         .vertexInputMode = Material::VertexInputMode::Mesh,
         .topology = vk::PrimitiveTopology::eTriangleList,
         .cullMode = vk::CullModeFlagBits::eBack,
         .depthTest = true,
         .depthWrite = true,
         .depthCompare = vk::CompareOp::eLessOrEqual,
         .blendMode = Material::BlendMode::Opaque,
         .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                           vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
         .descriptorSetBindings = {},
         .pushConstants = {vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants))}},
        {});

    getOrCreateMaterial("grid_overlay",
                        {.vertexShader = {.filePath = "shaders/gridVert.glsl", .stage = EShLangVertex},
                         .fragmentShader = {.filePath = "shaders/gridFrag.glsl", .stage = EShLangFragment},
                         .vertexInputMode = Material::VertexInputMode::None,
                         .topology = vk::PrimitiveTopology::eTriangleList,
                         .cullMode = vk::CullModeFlagBits::eNone,
                         .depthTest = true,
                         .depthWrite = false,
                         .depthCompare = vk::CompareOp::eLessOrEqual,
                         .blendMode = Material::BlendMode::Opaque,
                         .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                           vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
                         .descriptorSetBindings = {},
                         .pushConstants = {}},
                        {});

    auto defaultSamplerConfig = Vulkan::SamplerConfig{};
    auto whitePixelHandle = getOrCreateSampler(defaultSamplerConfig);

    auto context = Vulkan::UploadContext(device, uploadPool, *stagingBuffer);

    auto data = std::array<uint8_t, 4>({255, 255, 255, 255});

    auto texture = textures.insert(Vulkan::Texture(device, &data, 1, 1, whitePixelHandle));

    context.upload(*texture);
}

auto ResourceManager::extractTextureTransform(const tinygltf::ExtensionMap& extensions) -> Material::TextureTransform {
    Material::TextureTransform result;
    auto it = extensions.find("KHR_texture_transform");
    if (it == extensions.end()) {
        return result;
    }

    const auto& ext = it->second;
    if (!ext.IsObject()) {
        return result;
    }

    const auto& obj = ext.Get<tinygltf::Value::Object>();

    auto getVec2 =
        [](const tinygltf::Value& val, const tinygltf::Value::Object& obj, const std::string& key) -> glm::vec2 {
        auto kit = obj.find(key);
        if (kit != obj.end() && kit->second.IsArray()) {
            const auto& arr = kit->second.Get<tinygltf::Value::Array>();
            if (arr.size() >= 2) {
                return {static_cast<float>(arr[0].Get<double>()), static_cast<float>(arr[1].Get<double>())};
            }
        }
        return {0.0f, 0.0f};
    };

    auto getFloat =
        [](const tinygltf::Value& val, const tinygltf::Value::Object& obj, const std::string& key) -> float {
        auto kit = obj.find(key);
        if (kit != obj.end() && kit->second.IsNumber()) {
            return static_cast<float>(kit->second.GetNumberAsDouble());
        }
        return 0.0f;
    };

    result.offset = getVec2(ext, obj, "offset");
    result.rotation = getFloat(ext, obj, "rotation");
    result.scale = getVec2(ext, obj, "scale");

    // scale defaults to (1, 1) if missing
    auto scaleIt = obj.find("scale");
    if (scaleIt == obj.end() || !scaleIt->second.IsArray()) {
        result.scale = {1.0f, 1.0f};
    }

    return result;
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

void ResourceManager::onWantsPrimitive(entt::registry& registry, entt::entity entity) {
    auto want = registry.get<Scene::WantsPrimitive>(entity);
    auto mesh = uploadMesh(want.primitive->vertices, want.primitive->indices);
    want.primitive->uploadedMesh = mesh;
    tagRemovalQueue.emplace_back(entity);
    registry.emplace<Scene::Primitive>(entity, mesh, want.primitive->material);
}

void ResourceManager::onFrameTick(entt::registry& registry, entt::entity entity) {
    auto frame = registry.get<FrameContext>(entity).index;
    publicResources.currentFrameIndex = frame;
    publicResources.sceneDescriptorSet = publicResources.sceneDescriptorSets[frame];

    // Clear flags
    for (auto e : tagRemovalQueue) {
        if (registry.all_of<Scene::WantsPrimitive>(e)) {
            registry.remove<Scene::WantsPrimitive>(e);
        }
    }
    tagRemovalQueue.clear();

    // Do UBO updates
    {
        auto cameraView = registry.view<Scene::CameraSettings, Scene::DirtyUBOs, Scene::UBOArrayIndex>();
        for (auto [e, camera, dirtyUBOs, slot] : cameraView.each()) {
            if (dirtyUBOs.dirtyUBOs > 0) {
                (*cameraUniformBuffers[frame])[slot.index] = cameraUBOCache[slot.index];
                dirtyUBOs.dirtyUBOs -= 1;
            }
        }

        auto lightView = registry.view<Scene::LightSettings, Scene::DirtyUBOs, Scene::UBOArrayIndex>();
        for (auto [e, light, dirtyUBOs, slot] : lightView.each()) {
            if (dirtyUBOs.dirtyUBOs > 0) {
                (*lightUniformBuffers[frame])[slot.index] = lightUBOCache[slot.index];
                dirtyUBOs.dirtyUBOs -= 1;
            }
        }
    }
}

void ResourceManager::onCameraCreated(entt::registry& registry, entt::entity entity) {
    auto [transform, settings] = registry.get<Scene::CameraTransform, Scene::CameraSettings>(entity);
    // allocate a new camera in the buffers
    auto index = cameraUBOCache.allocate();
    // We can put in an index now
    registry.emplace<Scene::UBOArrayIndex>(entity, Scene::UBOArrayIndex{index});

    // Mark both UBOs dirty
    registry.emplace<Scene::DirtyUBOs>(entity, Scene::DirtyUBOs{2});

    for (auto& buffer : cameraUniformBuffers) {
        (*buffer)[buffer->allocate()];
    }

    cameraUBOCache[index].view = Scene::Camera::getViewMatrix(transform);
    cameraUBOCache[index].proj = Scene::Camera::getProjectionMatrix(settings);
    cameraUBOCache[index].invView = glm::inverse(cameraUBOCache[index].view);
    cameraUBOCache[index].invProj = glm::inverse(cameraUBOCache[index].proj);

    cameraIndexMap[index] = entity;
}

void ResourceManager::onCameraUpdated(entt::registry& registry, entt::entity entity) {
    auto [transform, settings, index] =
        registry.get<Scene::CameraTransform, Scene::CameraSettings, Scene::UBOArrayIndex>(entity);

    auto cameraIndex = index.index;

    cameraUBOCache[cameraIndex].view = Scene::Camera::getViewMatrix(transform);
    cameraUBOCache[cameraIndex].proj = Scene::Camera::getProjectionMatrix(settings);
    cameraUBOCache[cameraIndex].invView = glm::inverse(cameraUBOCache[cameraIndex].view);
    cameraUBOCache[cameraIndex].invProj = glm::inverse(cameraUBOCache[cameraIndex].proj);

    // Mark both UBOs dirty
    registry.emplace_or_replace<Scene::DirtyUBOs>(entity, Scene::DirtyUBOs{2});
}

void ResourceManager::onCameraDeleted(entt::registry& registry, entt::entity entity) {
    auto& index = registry.get<Scene::UBOArrayIndex>(entity);
    const auto cameraIndex = index.index;

    auto movedIndex = cameraUBOCache.free(cameraIndex);

    if (movedIndex.has_value() && cameraIndexMap.contains(movedIndex.value())) {
        auto movedEntity = cameraIndexMap[movedIndex.value()];
        registry.emplace_or_replace<Scene::UBOArrayIndex>(movedEntity, Scene::UBOArrayIndex{cameraIndex});
        cameraIndexMap.erase(movedIndex.value());
        cameraIndexMap[cameraIndex] = movedEntity;
    }

    cameraIndexMap.erase(cameraIndex);
}

void ResourceManager::onLightCreated(entt::registry& registry, entt::entity entity) {
    auto [transform, settings] = registry.get<Scene::LocalTransform, Scene::LightSettings>(entity);
    auto index = lightUBOCache.allocate();

    registry.emplace<Scene::UBOArrayIndex>(entity, Scene::UBOArrayIndex{index});

    // Mark both UBOs dirty
    registry.emplace<Scene::DirtyUBOs>(entity, Scene::DirtyUBOs{2});

    for (auto& buffer : lightUniformBuffers) {
        (*buffer)[buffer->allocate()];
    }

    lightUBOCache[index].colour = settings.colour;
    lightUBOCache[index].intensity = settings.intensity;
    lightUBOCache[index].maxRange = settings.maxRange;
    lightUBOCache[index].position = transform.position;

    lightIndexMap[index] = entity;
}

auto ResourceManager::importGLTF(const std::string& path) -> AssetHandle {
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

    // First pass: collect texture format assignments from materials
    std::unordered_map<int, vk::Format> textureFormats;

    auto assignFormat = [&](int texIndex, vk::Format format) -> void {
        if (texIndex >= 0) {
            textureFormats.try_emplace(texIndex, format);
        }
    };

    for (const auto& material : gltfModel.materials) {
        const auto& pbr = material.pbrMetallicRoughness;
        assignFormat(pbr.baseColorTexture.index, vk::Format::eR8G8B8A8Srgb);
        assignFormat(pbr.metallicRoughnessTexture.index, vk::Format::eR8G8B8A8Unorm);
        assignFormat(material.normalTexture.index, vk::Format::eR8G8B8A8Unorm);
        assignFormat(material.occlusionTexture.index, vk::Format::eR8G8B8A8Unorm);
        assignFormat(material.emissiveTexture.index, vk::Format::eR8G8B8A8Srgb);
    }

    // Upload a texture, defaulting to Unorm if no format was assigned
    auto uploadTexture = [&](int i, vk::Format format = vk::Format::eR8G8B8A8Unorm) -> TextureHandle {
        auto context = Vulkan::UploadContext(*device, uploadPool, *stagingBuffer);
        const auto& image = gltfModel.images[gltfModel.textures[i].source];
        auto sampler = getOrCreateSampler(Vulkan::SamplerConfig{});

        auto texture =
            textures.insert(Vulkan::Texture(*device, image.image.data(), image.width, image.height, sampler, format));

        context.upload(*texture);

        return texture;
    };

    // Upload all textures with their correct format (or default if unused by materials)
    std::unordered_map<int, TextureHandle> textureHandleMap;
    std::vector<TextureHandle> textureHandles;
    textureHandles.reserve(gltfModel.textures.size());

    for (auto i = 0; i < gltfModel.textures.size(); i++) {
        auto it = textureFormats.find(i);
        const vk::Format format = (it != textureFormats.end()) ? it->second : vk::Format::eR8G8B8A8Unorm;
        textureHandleMap[i] = textureHandles.emplace_back(uploadTexture(i, format));
    }

    std::vector<MaterialHandle> materialHandles;

    materialHandles.reserve(gltfModel.materials.size());
    for (auto& material : gltfModel.materials) {

        const auto& pbr = material.pbrMetallicRoughness;

        auto loadTexture = [&](const auto& texInfo,
                               TextureHandle& handle,
                               int32_t& uvIndex,
                               Material::TextureTransform& transform) -> auto {
            if (texInfo.index < 0) {
                handle = textures.makeHandle(0, 0);
                uvIndex = 0;
                transform = {};
                return;
            }
            handle = textureHandles[texInfo.index];
            uvIndex = texInfo.texCoord;
            transform = extractTextureTransform(texInfo.extensions);
        };

        Material::BlendMode blendMode = Material::BlendMode::Opaque;
        if (material.alphaMode == "BLEND" || material.alphaMode == "MASK") { // treat cutout as alpha for now
            blendMode = Material::BlendMode::Alpha;
        }

        const vk::CullModeFlagBits cullMode =
            material.doubleSided ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack;

        Material::MaterialResources resources;

        loadTexture(
            pbr.baseColorTexture, resources.baseColorTexture, resources.baseColorUVIndex, resources.baseColorTransform);
        loadTexture(pbr.metallicRoughnessTexture,
                    resources.metallicRoughnessTexture,
                    resources.metallicRoughnessUVIndex,
                    resources.metallicRoughnessTransform);
        loadTexture(
            material.normalTexture, resources.normalTexture, resources.normalUVIndex, resources.normalTransform);
        loadTexture(
            material.occlusionTexture, resources.occlusionTexture, resources.aoUVIndex, resources.occlusionTransform);
        loadTexture(material.emissiveTexture,
                    resources.emissiveTexture,
                    resources.emissiveUVIndex,
                    resources.emissiveTransform);

        resources.baseColorFactor = pbr.baseColorFactor;
        resources.emissiveFactor = material.emissiveFactor;
        resources.metallicFactor = static_cast<float>(pbr.metallicFactor);
        resources.roughnessFactor = static_cast<float>(pbr.roughnessFactor);

        auto mat = getOrCreateMaterial(
            material.name,
            {.vertexShader = {.filePath = "shaders/vert.glsl", .stage = EShLangVertex},
             .fragmentShader = {.filePath = "shaders/frag.glsl", .stage = EShLangFragment},
             .vertexInputMode = Material::VertexInputMode::Mesh,
             .topology = vk::PrimitiveTopology::eTriangleList,
             .cullMode = cullMode,
             .depthTest = true,
             .depthWrite = true,
             .depthCompare = vk::CompareOp::eLessOrEqual,
             .blendMode = blendMode,
             .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
             .descriptorSetBindings = {},
             .pushConstants = {vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants))}},
            resources);

        materialHandles.emplace_back(mat);
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
        const std::vector<float> texCoords0 = readAccessor("TEXCOORD_0");
        const std::vector<float> texCoords1 = readAccessor("TEXCOORD_1");
        const std::vector<float> colors = readAccessor("COLOR_0");

        const size_t vertexCount = gltfModel.accessors[primitive.attributes.at("POSITION")].count;
        vertices.resize(vertexCount);

        for (size_t i = 0; i < vertexCount; i++) {
            vertices[i].pos = {positions[i * 3], positions[(i * 3) + 1], positions[(i * 3) + 2]};
            vertices[i].normal = !normals.empty()
                                     ? glm::vec3(normals[i * 3], normals[(i * 3) + 1], normals[(i * 3) + 2])
                                     : glm::vec3(0, 1, 0);
            vertices[i].uv0 =
                !texCoords0.empty() ? glm::vec2(texCoords0[i * 2], texCoords0[(i * 2) + 1]) : glm::vec2(0, 0);
            vertices[i].uv1 =
                !texCoords1.empty() ? glm::vec2(texCoords1[i * 2], texCoords1[(i * 2) + 1]) : glm::vec2(0, 0);
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

    auto buildNode = [&](this auto& self, tinygltf::Node& gltfNode) -> glTFNode {
        glTFNode node{};

        if (!gltfNode.matrix.empty()) {
            glm::mat4 matrix;
            for (auto i = 0; i < 16; ++i) {
                matrix[i / 4][i % 4] = static_cast<float>(gltfNode.matrix[i]);
            }

            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(matrix, node.scale, node.rotation, node.translation, skew, perspective);
        } else {
            if (!gltfNode.translation.empty()) {
                node.translation = {static_cast<float>(gltfNode.translation[0]),
                                    static_cast<float>(gltfNode.translation[1]),
                                    static_cast<float>(gltfNode.translation[2])};
            } else {
                node.translation = glm::vec3(0.0f);
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
                auto materialHandle = materialHandles[gltfPrimitive.material];

                // We have a primitive
                // Now we need to put this in the registry
                node.primitives.emplace_back(
                    glTFPrimitive{.vertices = meshData.first, .indices = meshData.second, .material = materialHandle});
            }
        }

        node.name = gltfNode.name;

        // If we have a parent, put ourselves in the Children list too
        for (const auto& childID : gltfNode.children) {
            auto& gltfChild = gltfModel.nodes[childID];
            auto child = self(gltfChild);
            node.children.emplace_back(child);
        }

        return node;
    };

    AssetHandle handle;

    // Root nodes correspond to our "Objects"
    for (const auto& nodeIdx : gltfModel.scenes[gltfModel.defaultScene].nodes) {
        auto& rootNode = gltfModel.nodes[nodeIdx];

        handle = assets.insert(glTFObject(buildNode(rootNode)));
    }

    return handle;
}

auto ResourceManager::getPipelineManager() -> Vulkan::PipelineManager& { return pipelineManager; }
auto ResourceManager::getMaterial(const std::string& name) -> MaterialHandle { return materialNameMap[name]; }
auto ResourceManager::getResources() const -> const RenderResources& { return publicResources; }

auto ResourceManager::createRenderTarget(const std::string& name,
                                         vk::Extent3D extent,
                                         vk::Format format,
                                         vk::ImageUsageFlags usageFlags,
                                         vk::ImageAspectFlags aspectFlags,
                                         vk::SampleCountFlagBits sampleCount) -> RenderTargetHandle {

    auto handle = renderTargets.insert(RenderTarget(*device, extent, format, usageFlags, aspectFlags, sampleCount));

    renderTargetNameMap[name] = handle;

    return handle;
}

auto ResourceManager::getRenderTarget(const std::string& name) -> RenderTargetHandle {
    return renderTargetNameMap[name];
}

auto ResourceManager::getOrCreateSampler(const Vulkan::SamplerConfig& config) -> SamplerHandle {
    if (auto it = samplerIndex.find(config); it != samplerIndex.end()) {
        return it->second;
    }

    auto sampler = samplers.insert(Vulkan::Sampler(*device, config));
    samplerIndex[config] = sampler;
    return sampler;
}

auto ResourceManager::getOrCreateMaterial(const std::string& name,
                                          const Material::MaterialConfiguration& config,
                                          const Material::MaterialResources& resources) -> MaterialHandle {
    // Do we maybe know it by name
    auto handleByName = materialNameMap.find(name);

    if (handleByName != materialNameMap.end()) {
        return handleByName->second;
    }

    // By configuration then
    auto key = Material::MaterialKey{.config = config, .resources = resources};

    auto handleByConfig = materialConfigMap.find(key);

    if (handleByConfig != materialConfigMap.end()) {
        // We didn't match by name, but the setups are the same... Opportunity to deduplicate?
        return handleByConfig->second;
    }

    // Make a new material
    Material mat = {name, config, resources};

    mat.materialBufferHandle = materialUniformBuffers.insert(Vulkan::HostBuffer<MaterialUBO>(*device));

    auto insertedHandle = materials.insert(std::move(mat));

    materialNameMap.emplace(name, insertedHandle);
    materialConfigMap.emplace(key, insertedHandle);

    pipelineManager.registerMaterial(*insertedHandle);

    return insertedHandle;
}
} // namespace Render
