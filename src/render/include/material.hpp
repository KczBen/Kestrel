#pragma once

#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "graphicsPipeline.hpp"
#include "hashHelpers.hpp"
#include "hostBuffer.hpp"
#include "resourceHandles.hpp"
#include "shaderDef.hpp"
#include "texture.hpp"
#include "vulkan/vulkan.hpp"
#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>
#include <string>
#include <vector>

namespace Vulkan {
class PipelineManager;
}

namespace Render {
class ResourceManager;
}

class Material {
  public:
    enum class BlendMode : uint8_t { Opaque, Alpha, Premultiplied, Additive };
    enum class VertexInputMode : uint8_t { None, Mesh, Instanced };

    struct TextureTransform {
        glm::vec2 offset{0.0f, 0.0f};
        float rotation{0.0f};
        glm::vec2 scale{1.0f, 1.0f};

        auto operator==(const TextureTransform& other) const -> bool {
            return offset == other.offset && rotation == other.rotation && scale == other.scale;
        }
    };

    // This should be compatible with the glTF 2.0 spec
    struct MaterialResources {
        // Textures
        TextureHandle baseColorTexture;
        TextureHandle metallicRoughnessTexture;
        TextureHandle normalTexture;
        TextureHandle occlusionTexture;
        TextureHandle emissiveTexture;

        // Texture UV indices (which vertex attribute to sample from)
        int32_t baseColorUVIndex{0};
        int32_t metallicRoughnessUVIndex{0};
        int32_t normalUVIndex{0};
        int32_t aoUVIndex{0};
        int32_t emissiveUVIndex{0};

        // Texture Transforms (KHR_texture_transform)
        TextureTransform baseColorTransform;
        TextureTransform metallicRoughnessTransform;
        TextureTransform normalTransform;
        TextureTransform occlusionTransform;
        TextureTransform emissiveTransform;

        // glTF Factors / Uniforms
        std::vector<double> baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        std::vector<double> emissiveFactor{0.0f, 0.0f, 0.0f};
        float metallicFactor{1.0f};
        float roughnessFactor{1.0f};
        float normalScale{1.0f};
        float occlusionStrength{1.0f};

        auto operator==(const MaterialResources& other) const -> bool;

        [[nodiscard]] auto isEmpty() const -> bool {
            return !baseColorTexture.isPresent() && !metallicRoughnessTexture.isPresent() &&
                   !normalTexture.isPresent() && !occlusionTexture.isPresent() && !emissiveTexture.isPresent();
        };
    };

    struct MaterialConfiguration {
        ShaderDef vertexShader;
        ShaderDef fragmentShader;

        VertexInputMode vertexInputMode;

        vk::PrimitiveTopology topology;
        vk::CullModeFlagBits cullMode;

        bool depthTest;
        bool depthWrite;
        vk::CompareOp depthCompare;

        BlendMode blendMode;
        vk::ColorComponentFlags colorWriteMask;

        std::vector<vk::DescriptorSetLayoutBinding> descriptorSetBindings;
        std::vector<vk::PushConstantRange> pushConstants;

        auto operator==(const MaterialConfiguration& other) const -> bool;
    };

    struct MaterialKey {
        Material::MaterialConfiguration config;
        Material::MaterialResources resources;

        auto operator==(const MaterialKey& other) const -> bool {
            return config == other.config && resources == other.resources;
        }
    };

    Material(std::string name, MaterialConfiguration config, MaterialResources resources);

    auto getDescriptorSet() -> Vulkan::DescriptorSet&;
    auto getDescriptorSetLayout() -> std::optional<Vulkan::DescriptorSetLayout>&;
    [[nodiscard]] auto getName() const -> const std::string& { return name; }
    [[nodiscard]] auto getResources() const -> const MaterialResources& { return resources; }
    [[nodiscard]] auto getConfiguration() const -> const MaterialConfiguration& { return pipelineConfiguration; }
    [[nodiscard]] auto getBlendMode() const -> BlendMode { return pipelineConfiguration.blendMode; }

  private:
    std::string name;
    MaterialResources resources;
    MaterialConfiguration pipelineConfiguration;

    std::optional<Vulkan::DescriptorSetLayout> descriptorSetLayout;
    std::optional<Vulkan::DescriptorSet> descriptorSet;
    Vulkan::MaterialUBOHandle materialBufferHandle;
    Vulkan::GraphicsPipeline* graphicsPipeline = nullptr;

    friend class Vulkan::PipelineManager;
    friend class Render::ResourceManager;
};

inline auto buildTextureTransformMatrix(const Material::TextureTransform& transform) -> glm::mat4 {
    const float c = glm::cos(transform.rotation);
    const float s = glm::sin(transform.rotation);

    glm::mat4 m(1.0f);
    m[0][0] = transform.scale.x * c;
    m[0][1] = transform.scale.x * s;
    m[1][0] = -transform.scale.y * s;
    m[1][1] = transform.scale.y * c;
    m[3][0] = transform.offset.x;
    m[3][1] = transform.offset.y;
    return m;
}

using MaterialHandle = Render::ResourceHandle<Material>;

namespace std {
template <> struct hash<Material::TextureTransform> {
    auto operator()(const Material::TextureTransform& transform) const noexcept -> size_t {
        size_t seed = 0;
        hashCombine(seed, transform.offset); // uses hash<glm::vec2> above
        hashCombine(seed, transform.rotation);
        hashCombine(seed, transform.scale);
        return seed;
    }
};

template <> struct hash<Material::MaterialConfiguration> {
    auto operator()(const Material::MaterialConfiguration& config) const noexcept -> size_t {
        size_t seed = 0;
        hashCombine(seed, config.vertexShader);
        hashCombine(seed, config.fragmentShader);
        hashCombine(seed, static_cast<uint8_t>(config.vertexInputMode));
        hashCombine(seed, static_cast<uint32_t>(config.topology));
        hashCombine(seed, static_cast<uint32_t>(config.cullMode));
        hashCombine(seed, config.depthTest);
        hashCombine(seed, config.depthWrite);
        hashCombine(seed, static_cast<uint32_t>(config.depthCompare));
        hashCombine(seed, static_cast<uint8_t>(config.blendMode));
        hashCombine(seed, static_cast<VkColorComponentFlags>(config.colorWriteMask));

        for (const auto& binding : config.descriptorSetBindings) {
            hashCombine(seed, binding.binding);
            hashCombine(seed, static_cast<uint32_t>(binding.descriptorType));
            hashCombine(seed, binding.descriptorCount);
            hashCombine(seed, static_cast<VkShaderStageFlags>(binding.stageFlags));
        }
        for (const auto& range : config.pushConstants) {
            hashCombine(seed, static_cast<VkShaderStageFlags>(range.stageFlags));
            hashCombine(seed, range.offset);
            hashCombine(seed, range.size);
        }
        return seed;
    }
};

template <> struct hash<Material::MaterialResources> {
    auto operator()(const Material::MaterialResources& res) const noexcept -> size_t {
        size_t seed = 0;
        hashCombine(seed, res.baseColorTexture);
        hashCombine(seed, res.metallicRoughnessTexture);
        hashCombine(seed, res.normalTexture);
        hashCombine(seed, res.occlusionTexture);
        hashCombine(seed, res.emissiveTexture);

        hashCombine(seed, res.baseColorUVIndex);
        hashCombine(seed, res.metallicRoughnessUVIndex);
        hashCombine(seed, res.normalUVIndex);
        hashCombine(seed, res.aoUVIndex);
        hashCombine(seed, res.emissiveUVIndex);

        hashCombine(seed, res.baseColorTransform);
        hashCombine(seed, res.metallicRoughnessTransform);
        hashCombine(seed, res.normalTransform);
        hashCombine(seed, res.occlusionTransform);
        hashCombine(seed, res.emissiveTransform);

        for (const double v : res.baseColorFactor) {
            hashCombine(seed, v);
        }
        for (const double v : res.emissiveFactor) {
            hashCombine(seed, v);
        }

        hashCombine(seed, res.metallicFactor);
        hashCombine(seed, res.roughnessFactor);
        hashCombine(seed, res.normalScale);
        hashCombine(seed, res.occlusionStrength);

        return seed;
    }
};

template <> struct hash<Material::MaterialKey> {
    auto operator()(const Material::MaterialKey& key) const noexcept -> size_t {
        size_t seed = 0;
        hashCombine(seed, key.config);
        hashCombine(seed, key.resources);

        return seed;
    }
};
} // namespace std
