#pragma once

#include "descriptorPool.hpp"
#include "device.hpp"
#include "graphicsPipeline.hpp"
#include "render/material.hpp"
#include <span>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

struct PipelineKey : Material::MaterialConfiguration {
    PipelineKey(const MaterialConfiguration& config,
                std::span<const vk::Format> formats,
                vk::SampleCountFlagBits samplesPerPixel,
                bool depthWrite)
        : MaterialConfiguration(config), formats(formats.begin(), formats.end()), samplesPerPixel(samplesPerPixel),
          depthWrite(depthWrite) {}
    std::vector<vk::Format> formats;
    vk::SampleCountFlagBits samplesPerPixel;
    bool depthWrite;

    [[nodiscard]] auto hash() const -> uint64_t;

    auto operator==(const PipelineKey& other) const -> bool {
        return MaterialConfiguration::operator==(other) && formats.size() == other.formats.size() &&
               samplesPerPixel == other.samplesPerPixel &&
               std::equal(formats.begin(), formats.end(), other.formats.begin()) && depthWrite == other.depthWrite;
    }
};

namespace std {
template <> struct hash<PipelineKey> {
    auto operator()(const PipelineKey& key) const noexcept -> size_t {
        auto baseHash = std::hash<Material::MaterialConfiguration>{}(key);
        auto formatHash = std::hash<uint32_t>{}(static_cast<uint32_t>(key.formats.size()));
        auto sampleHash = std::hash<uint32_t>{}(static_cast<uint32_t>(key.samplesPerPixel));
        size_t combined = baseHash ^ (formatHash << 1) ^ (sampleHash << 2);
        for (const auto& fmt : key.formats) {
            combined ^=
                std::hash<uint32_t>{}(static_cast<uint32_t>(fmt)) + 0x9e3779b9 + (combined << 6) + (combined >> 2);
        }
        return combined;
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
    auto getOrCreatePipeline(MaterialHandle materialHandle,
                             std::span<const Vulkan::DescriptorSetLayout*> layouts,
                             std::span<const vk::Format> formats,
                             vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1,
                             bool depthWrite = true) -> Vulkan::GraphicsPipeline&;
};
} // namespace Vulkan