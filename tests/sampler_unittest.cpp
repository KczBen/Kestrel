#include "hostBuffer.hpp"
#include "resourceHandles.hpp"
#include "sampler.hpp"
#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

using namespace Vulkan;

TEST(SamplerTest, DefaultSamplerUsesRepeatAndLinear) {
    // The default sampler config should use linear filter and repeat address mode
    SamplerConfig config;
    EXPECT_EQ(config.minFilter, vk::Filter::eLinear);
    EXPECT_EQ(config.magFilter, vk::Filter::eLinear);
    EXPECT_EQ(config.mipmapMode, vk::SamplerMipmapMode::eLinear);
    EXPECT_EQ(config.addressModeU, vk::SamplerAddressMode::eRepeat);
    EXPECT_EQ(config.addressModeV, vk::SamplerAddressMode::eRepeat);
    EXPECT_EQ(config.addressModeW, vk::SamplerAddressMode::eRepeat);
    EXPECT_FALSE(config.anisotropyEnable);
}

TEST(SamplerTest, SamplerConfigIsEqual) {
    SamplerConfig config1;
    config1.anisotropyEnable = VK_TRUE;

    SamplerConfig config2;
    config2.anisotropyEnable = VK_TRUE;

    EXPECT_TRUE(config1 == config2);
}

TEST(SamplerTest, SamplerConfigIsNotEqual) {
    SamplerConfig config1;
    config1.addressModeU = vk::SamplerAddressMode::eClampToEdge;

    SamplerConfig config2;
    config2.addressModeU = vk::SamplerAddressMode::eRepeat;

    EXPECT_FALSE(config1 == config2);
}

TEST(SamplerTest, SamplerHandleIsDefaultConstructed) {
    SamplerHandle handle;
    EXPECT_FALSE(handle.isPresent());
}

TEST(SamplerTest, SamplerConfigWithAnisotropy) {
    SamplerConfig config;
    config.anisotropyEnable = VK_TRUE;
    config.maxAnisotropy = 16.0f;

    SamplerConfig config2;
    config2.anisotropyEnable = VK_TRUE;
    config2.maxAnisotropy = 16.0f;

    EXPECT_TRUE(config == config2);
}

TEST(SamplerTest, SamplerConfigWithClampModes) {
    SamplerConfig config;
    config.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    config.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    config.addressModeW = vk::SamplerAddressMode::eMirrorClampToEdge;

    SamplerConfig config2;
    config2.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    config2.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    config2.addressModeW = vk::SamplerAddressMode::eMirrorClampToEdge;

    EXPECT_TRUE(config == config2);
}
