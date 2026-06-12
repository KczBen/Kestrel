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
    Material::MaterialResources resources;
    Material mat{vertexDef, fragmentDef, resources};
    EXPECT_EQ(mat.getBlendMode(), Material::BlendMode::Opaque);
}

TEST(MaterialTest, CreateMaterialWithAlphaBlend) {
    ShaderDef vertexDef{"shaders/shader.vert", EShLanguage::EShLangVertex};
    ShaderDef fragmentDef{"shaders/shader.frag", EShLanguage::EShLangFragment};
    Material::MaterialResources resources;
    Material mat{vertexDef,
                 fragmentDef,
                 resources,
                 {},
                 Material::VertexInputMode::Mesh,
                 vk::CullModeFlagBits::eBack,
                 true,
                 true,
                 vk::CompareOp::eLess,
                 Material::BlendMode::Alpha};
    EXPECT_EQ(mat.getBlendMode(), Material::BlendMode::Alpha);
}

TEST(MaterialTest, MaterialResourcesIsEmpty) {
    Material::MaterialResources resources;
    EXPECT_TRUE(resources.isEmpty());
}
