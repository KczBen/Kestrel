#pragma once

#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "gpuDataObjects.hpp"
#include "graphicsPipeline.hpp"
#include "hostBuffer.hpp"
#include "resourceHandles.hpp"
#include "shaderDef.hpp"
#include "texture.hpp"
#include "vulkan/vulkan.hpp"
#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>
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

        [[nodiscard]] auto hash() const -> uint64_t;
        auto operator==(const MaterialConfiguration&) const -> bool;
    };

    Material(ShaderDef vertexShader,
             ShaderDef fragmentShader,
             const MaterialResources& materialResources,
             std::vector<vk::PushConstantRange> pushConstants = {},
             VertexInputMode vertexInputMode = VertexInputMode::Mesh,
             vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eBack,
             bool depthTest = true,
             bool depthWrite = true,
             vk::CompareOp depthCompare = vk::CompareOp::eLessOrEqual,
             BlendMode colorBlendMode = BlendMode::Opaque,
             vk::ColorComponentFlags colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    auto getDescriptorSet() -> Vulkan::DescriptorSet&;
    [[nodiscard]] auto getResources() const -> const MaterialResources& { return resources; }
    [[nodiscard]] auto getBlendMode() const -> BlendMode { return pipelineConfiguration.blendMode; }

  private:
    MaterialResources resources;
    MaterialConfiguration pipelineConfiguration;

    std::optional<Vulkan::DescriptorSetLayout> descriptorSetLayout;
    std::optional<Vulkan::DescriptorSet> descriptorSet;
    Vulkan::HostBufferHandle materialBufferHandle;
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
        hashCombine(seed, static_cast<uint32_t>(config.colorWriteMask));
        hashCombine(seed, config.descriptorSetBindings.size());
        hashCombine(seed, config.pushConstants.size());
        return seed;
    }

  private:
    template <typename T> static void hashCombine(size_t& seed, const T& value) noexcept {
        seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};
} // namespace std