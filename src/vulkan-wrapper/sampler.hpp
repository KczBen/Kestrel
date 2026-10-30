#pragma once

#include "device.hpp"
#include "util/resourceHandles.hpp"

#include <functional>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
struct SamplerConfig {
    vk::Filter minFilter = vk::Filter::eLinear;
    vk::Filter magFilter = vk::Filter::eLinear;
    vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;
    vk::SamplerAddressMode addressModeU = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode addressModeV = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode addressModeW = vk::SamplerAddressMode::eRepeat;
    float mipBias = 0.0f;
    bool anisotropyEnable = VK_FALSE;
    float maxAnisotropy = 1.0f;
    vk::BorderColor borderColor = vk::BorderColor::eIntOpaqueBlack;

    auto operator==(const SamplerConfig& other) const -> bool {
        return minFilter == other.minFilter && magFilter == other.magFilter && mipmapMode == other.mipmapMode &&
               addressModeU == other.addressModeU && addressModeV == other.addressModeV &&
               addressModeW == other.addressModeW && mipBias == other.mipBias &&
               anisotropyEnable == other.anisotropyEnable && maxAnisotropy == other.maxAnisotropy &&
               borderColor == other.borderColor;
    }
};

class Sampler {
  private:
    std::optional<vk::raii::Sampler> sampler;

  public:
    Sampler(Device& device, const SamplerConfig& config);

    [[nodiscard]] auto operator*() const -> const vk::raii::Sampler& { return sampler.value(); }
    [[nodiscard]] auto operator->() const -> const vk::raii::Sampler* { return &sampler.value(); }
    [[nodiscard]] auto get() const -> const vk::raii::Sampler&;
};

} // namespace Vulkan

template <> struct std::hash<Vulkan::SamplerConfig> {
    auto operator()(const Vulkan::SamplerConfig& config) const -> size_t {
        size_t h = 0;
        auto combine = [&](size_t& seed, size_t value) -> void {
            seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        combine(h, static_cast<size_t>(config.minFilter));
        combine(h, static_cast<size_t>(config.magFilter));
        combine(h, static_cast<size_t>(config.mipmapMode));
        combine(h, static_cast<size_t>(config.addressModeU));
        combine(h, static_cast<size_t>(config.addressModeV));
        combine(h, static_cast<size_t>(config.addressModeW));
        combine(h, std::hash<float>{}(config.mipBias));
        combine(h, std::hash<bool>{}(config.anisotropyEnable));
        combine(h, std::hash<float>{}(config.maxAnisotropy));
        combine(h, static_cast<size_t>(config.borderColor));

        return h;
    }
};

using SamplerHandle = ResourceHandle<Vulkan::Sampler>;