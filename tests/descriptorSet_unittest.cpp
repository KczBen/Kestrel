#include "descriptorPool.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "device.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "surface.hpp"
#include <gtest/gtest.h>

using namespace Vulkan;

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

TEST_F(DescriptorSetLayoutFixture, GetReturnsNonNull) { ASSERT_NE(*layout.get(), VK_NULL_HANDLE); }

class DescriptorSetFixture : public DescriptorSetLayoutFixture {
  protected:
    DescriptorPool pool{device, 2};
    DescriptorSet descriptorSet{device, pool, layout};
};

TEST_F(DescriptorSetFixture, GetReturnsNonNull) { ASSERT_NE(*descriptorSet.get(), VK_NULL_HANDLE); }
