#pragma once

#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "graphicsPipeline.hpp"
#include "object.hpp"
#include "shaderDef.hpp"
#include "texture.hpp"
#include "vulkan/vulkan.hpp"
#include <cstddef>
#include <optional>
#include <vector>

namespace Vulkan {
class PipelineManager;
}

class Material {
  public:
    enum class BlendMode : uint8_t { Opaque, Alpha, Premultiplied, Additive };
    enum class VertexInputMode : uint8_t { None, Mesh, Instanced };

    // This should be compatible with the glTF 2.0 spec
    struct MaterialResources {
        Vulkan::Texture* baseColor;
        Vulkan::Texture* metallicRoughness;
        Vulkan::Texture* normal;
        Vulkan::Texture* occlusion;
        Vulkan::Texture* emissive;

        [[nodiscard]] auto isEmpty() const -> bool {
            return baseColor == nullptr && metallicRoughness == nullptr && normal == nullptr && occlusion == nullptr &&
                   emissive == nullptr;
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
             Vulkan::Texture* baseColor = nullptr,
             Vulkan::Texture* metallicRoughness = nullptr,
             Vulkan::Texture* normal = nullptr,
             Vulkan::Texture* occlusion = nullptr,
             Vulkan::Texture* emissive = nullptr,
             std::vector<vk::PushConstantRange> pushConstants = {},
             VertexInputMode vertexInputMode = VertexInputMode::Mesh,
             vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eBack,
             bool depthTest = true,
             bool depthWrite = true,
             vk::CompareOp depthCompare = vk::CompareOp::eLess,
             BlendMode colorBlendMode = BlendMode::Opaque,
             vk::ColorComponentFlags colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    auto getDescriptorSet() -> Vulkan::DescriptorSet&;

  private:
    MaterialResources resources;
    MaterialConfiguration pipelineConfiguration;

    std::optional<Vulkan::DescriptorSetLayout> descriptorSetLayout;
    std::optional<Vulkan::DescriptorSet> descriptorSet;
    Vulkan::GraphicsPipeline* graphicsPipeline = nullptr;

    // Reduce the number of hash map lookups we need to do
    vk::Format lastRenderedFormat = vk::Format::eUndefined;

    friend class Vulkan::PipelineManager;
};

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