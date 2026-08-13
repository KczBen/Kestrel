#include "descriptorPool.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "device.hpp"
#include "hostBuffer.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include <gtest/gtest.h>

using namespace Vulkan;

namespace {
struct TestUBO {
    float data[16];
};
struct TestSSBO {
    float data[64];
};
} // namespace

// Shared base for anything that needs a Device
class DeviceFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
    Surface surface{instance};
    PhysicalDevice physicalDevice{instance, surface};
    Device device{physicalDevice, instance};
};

class DescriptorSetLayoutFixture : public DeviceFixture {
  protected:
    // A minimal UBO binding — valid on all compliant devices
    std::vector<vk::DescriptorSetLayoutBinding> bindings{
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex)};
    DescriptorSetLayout layout{device, bindings};
};

TEST_F(DescriptorSetLayoutFixture, GetReturnsNonNull) { ASSERT_NE(*layout, VK_NULL_HANDLE); }

class DescriptorSetFixture : public DescriptorSetLayoutFixture {
  protected:
    DescriptorPool pool{device};
    DescriptorSet descriptorSet{device, pool, layout};
};

TEST_F(DescriptorSetFixture, GetReturnsNonNull) { ASSERT_NE(*descriptorSet, VK_NULL_HANDLE); }

// Covers the eUniformBuffer branch in write()
TEST_F(DescriptorSetFixture, WriteUniformBuffer) {
    HostBuffer<TestUBO> ubo{device};

    const vk::DescriptorBufferInfo bufferInfo{ubo.get(), 0, sizeof(TestUBO)};

    DescriptorWrite write{};
    write.binding = 0;
    write.type = vk::DescriptorType::eUniformBuffer;
    write.bufferInfo = bufferInfo;

    // Should not throw — validates the buffer branch in write()
    EXPECT_NO_THROW(descriptorSet.write(std::array{write}));
}

// Covers the eStorageBuffer branch in write() — requires a layout with a storage binding
class DescriptorSetStorageFixture : public DeviceFixture {
  protected:
    std::vector<vk::DescriptorSetLayoutBinding> bindings{
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute)};
    DescriptorSetLayout layout{device, bindings};
    DescriptorPool pool{device};
    DescriptorSet descriptorSet{device, pool, layout};
};

TEST_F(DescriptorSetStorageFixture, WriteStorageBuffer) {
    HostBuffer<TestSSBO> ssbo{device, vk::BufferUsageFlagBits::eStorageBuffer};

    const vk::DescriptorBufferInfo bufferInfo{ssbo.get(), 0, sizeof(TestSSBO)};

    DescriptorWrite write{};
    write.binding = 0;
    write.type = vk::DescriptorType::eStorageBuffer;
    write.bufferInfo = bufferInfo;

    EXPECT_NO_THROW(descriptorSet.write(std::array{write}));
}

// Covers the image/sampler branch in write() (the else path)
class DescriptorSetSamplerFixture : public DeviceFixture {
  protected:
    std::vector<vk::DescriptorSetLayoutBinding> bindings{vk::DescriptorSetLayoutBinding(
        0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment)};
    DescriptorSetLayout layout{device, bindings};
    DescriptorPool pool{device};
    DescriptorSet descriptorSet{device, pool, layout};
};

TEST_F(DescriptorSetSamplerFixture, WriteImageSampler) {
    Image img{device,
              {64, 64, 1},
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageUsageFlagBits::eSampled,
              vk::ImageAspectFlagBits::eColor};

    const vk::SamplerCreateInfo samplerInfo{{},
                                            vk::Filter::eLinear,
                                            vk::Filter::eLinear,
                                            vk::SamplerMipmapMode::eLinear,
                                            vk::SamplerAddressMode::eRepeat,
                                            vk::SamplerAddressMode::eRepeat,
                                            vk::SamplerAddressMode::eRepeat};
    auto sampler = device->createSampler(samplerInfo);

    const vk::DescriptorImageInfo imageInfo{sampler, img.getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal};

    DescriptorWrite write{};
    write.binding = 0;
    write.type = vk::DescriptorType::eCombinedImageSampler;
    write.imageInfo = imageInfo;

    EXPECT_NO_THROW(descriptorSet.write(std::array{write}));
}