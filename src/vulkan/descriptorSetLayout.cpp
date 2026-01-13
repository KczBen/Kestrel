#include "descriptorSetLayout.hpp"
#include <vulkan/vulkan_raii.hpp>

Vulkan::DescriptorSetLayout::DescriptorSetLayout(Vulkan::Device& device,
                                                 std::span<const vk::DescriptorSetLayoutBinding> bindings)
    : device(&device.get()) {
    // const std::array bindings = {
    //     vk::DescriptorSetLayoutBinding(0,
    //                                    vk::DescriptorType::eUniformBuffer,
    //                                    1,
    //                                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
    //                                    nullptr),
    //     vk::DescriptorSetLayoutBinding(
    //         1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr)};

    const vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings.size(), bindings.data());

    layout = vk::raii::DescriptorSetLayout(*this->device, layoutInfo);
}

auto Vulkan::DescriptorSetLayout::get() const -> const vk::raii::DescriptorSetLayout& { return layout; }