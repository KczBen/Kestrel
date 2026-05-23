#pragma once

#include "camera.hpp"
#include "descriptorPool.hpp"
#include "device.hpp"
#include "graphicsPipeline.hpp"
#include "material.hpp"
#include "vulkan/vulkan.hpp"
#include <unordered_map>
#include <vulkan/vulkan_raii.hpp>

struct PipelineKey : Material::MaterialConfiguration {
    // just to make clang-tidy happy
    PipelineKey(const MaterialConfiguration& config, vk::Format format)
        : MaterialConfiguration(config), renderTargetFormat(format) {}
    vk::Format renderTargetFormat;

    [[nodiscard]] auto hash() const -> uint64_t;

    auto operator==(const PipelineKey& other) const -> bool {
        return MaterialConfiguration::operator==(other) && renderTargetFormat == other.renderTargetFormat;
    }
};

namespace std {
template <> struct hash<PipelineKey> {
    auto operator()(const PipelineKey& key) const noexcept -> size_t {
        auto baseHash = std::hash<Material::MaterialConfiguration>{}(key);
        auto formatHash = std::hash<uint32_t>{}(static_cast<uint32_t>(key.renderTargetFormat));
        return baseHash ^ (formatHash << 1);
    }
};
} // namespace std

namespace Vulkan {
class PipelineManager {
  private:
    Vulkan::Device* device;
    Vulkan::DescriptorPool* pool;

    // DESIGN: Maybe not hard-code this?
    vk::PushConstantRange pushConstants;

    // Do we already have a full pipeline for this?
    std::unordered_map<PipelineKey, GraphicsPipeline> pipelineCache;

    // TODO: Graphics pipeline library things
    // Can we reuse any of the pipeline libraries?
    // map<VertexSetup, vk::GraphicsPipelineLibrary> vertexStages;

    // same for all the GraphicsPipelineLibrary stages

  public:
    PipelineManager(Vulkan::Device& device, Vulkan::DescriptorPool& pool);

    void registerMaterial(Material& material);
    auto getOrCreatePipeline(MaterialHandle materialHandle, Render::Camera& camera, vk::Format format)
        -> Vulkan::GraphicsPipeline&;
};
} // namespace Vulkan