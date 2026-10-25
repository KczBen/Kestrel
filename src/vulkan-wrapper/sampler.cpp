#include "sampler.hpp"

#include <cassert>

Vulkan::Sampler::Sampler(Device& device, const SamplerConfig& config) {
    const vk::SamplerCreateInfo info({},
                                     config.minFilter,
                                     config.magFilter,
                                     config.mipmapMode,
                                     config.addressModeU,
                                     config.addressModeV,
                                     config.addressModeW,
                                     config.mipBias,
                                     static_cast<vk::Bool32>(config.anisotropyEnable),
                                     config.anisotropyEnable
                                         ? config.maxAnisotropy
                                         : device.getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy,
                                     vk::False,
                                     vk::CompareOp::eAlways,
                                     0.0f,
                                     vk::LodClampNone,
                                     config.borderColor,
                                     vk::False);

    sampler = vk::raii::Sampler(*device, info);
}

auto Vulkan::Sampler::get() const -> const vk::raii::Sampler& {
    assert(sampler.has_value());
    return sampler.value();
}
