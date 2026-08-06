#include "device.hpp"
#include "hostBuffer.hpp"
#include "instance.hpp"
#include "material.hpp"
#include "physicalDevice.hpp"
#include "resourceHandles.hpp"
#include "shaderDef.hpp"
#include "surface.hpp"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

using namespace Vulkan;

TEST(MaterialTest, CreateMaterialWithDefaults) {
    ShaderDef vertexDef{"shaders/shader.vert", EShLanguage::EShLangVertex};
    ShaderDef fragmentDef{"shaders/shader.frag", EShLanguage::EShLangFragment};
    Material::MaterialConfiguration config{.vertexShader = vertexDef,
                                           .fragmentShader = fragmentDef,
                                           .vertexInputMode = Material::VertexInputMode::Mesh,
                                           .topology = vk::PrimitiveTopology::eTriangleList,
                                           .cullMode = vk::CullModeFlagBits::eBack,
                                           .depthTest = true,
                                           .depthWrite = true,
                                           .depthCompare = vk::CompareOp::eLessOrEqual,
                                           .blendMode = Material::BlendMode::Opaque,
                                           .colorWriteMask =
                                               vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
                                           .descriptorSetBindings = {},
                                           .pushConstants = {}};
    Material::MaterialResources resources;
    Material mat{"test_material", config, resources};
    EXPECT_EQ(mat.getBlendMode(), Material::BlendMode::Opaque);
}

TEST(MaterialTest, CreateMaterialWithAlphaBlend) {
    ShaderDef vertexDef{"shaders/shader.vert", EShLanguage::EShLangVertex};
    ShaderDef fragmentDef{"shaders/shader.frag", EShLanguage::EShLangFragment};
    Material::MaterialConfiguration config{.vertexShader = vertexDef,
                                           .fragmentShader = fragmentDef,
                                           .vertexInputMode = Material::VertexInputMode::Mesh,
                                           .topology = vk::PrimitiveTopology::eTriangleList,
                                           .cullMode = vk::CullModeFlagBits::eBack,
                                           .depthTest = true,
                                           .depthWrite = true,
                                           .depthCompare = vk::CompareOp::eLess,
                                           .blendMode = Material::BlendMode::Alpha,
                                           .colorWriteMask =
                                               vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
                                           .descriptorSetBindings = {},
                                           .pushConstants = {}};
    Material::MaterialResources resources;
    Material mat{"test_material", config, resources};
    EXPECT_EQ(mat.getBlendMode(), Material::BlendMode::Alpha);
}

TEST(MaterialTest, MaterialResourcesIsEmpty) {
    Material::MaterialResources resources;
    EXPECT_TRUE(resources.isEmpty());
}
