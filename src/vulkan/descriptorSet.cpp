#include "descriptorSet.hpp"
#include "descriptorPool.hpp"
#include "descriptorSetLayout.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>

Vulkan::DescriptorSet::DescriptorSet(Vulkan::Device& device,
                                     Vulkan::DescriptorPool& pool,
                                     Vulkan::DescriptorSetLayout& layout)
    : device(&*device) {
    const vk::DescriptorSetLayout rawLayout = *layout;

    const vk::DescriptorSetAllocateInfo allocInfo{*pool, 1, &rawLayout};

    auto sets = this->device->allocateDescriptorSets(allocInfo);
    descriptorSet = std::move(sets.front());
}

void Vulkan::DescriptorSet::write(std::span<const DescriptorWrite> writes) {
    std::vector<vk::WriteDescriptorSet> vkWrites;
    vkWrites.reserve(writes.size());

    for (const auto& w : writes) {
        vk::WriteDescriptorSet write{};
        write.dstSet = descriptorSet;
        write.dstBinding = w.binding;
        write.descriptorCount = 1;
        write.descriptorType = w.type;

        if (w.type == vk::DescriptorType::eUniformBuffer || w.type == vk::DescriptorType::eStorageBuffer) {
            write.pBufferInfo = &w.bufferInfo;
        } else {
            write.pImageInfo = &w.imageInfo;
        }

        vkWrites.push_back(write);
    }

    device->updateDescriptorSets(vkWrites, {});
}

auto Vulkan::DescriptorSet::get() const -> const vk::raii::DescriptorSet& { return descriptorSet; }