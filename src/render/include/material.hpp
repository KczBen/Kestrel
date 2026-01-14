#include "descriptorPool.hpp"
#include "descriptorSet.hpp"
#include "descriptorSetLayout.hpp"
#include "graphicsPipeline.hpp"
#include "texture.hpp"

class Material {
  private:
    Vulkan::Texture* albedo;
    Vulkan::Texture* packedNormal;

    std::vector<Vulkan::DescriptorSet> descriptorSets;

    Vulkan::GraphicsPipeline* pipeline;

  public:
    Material(Vulkan::Device& device,
             Vulkan::Texture& albedo,
             Vulkan::Texture& packedNormal,
             Vulkan::GraphicsPipeline& pipeline,
             Vulkan::DescriptorPool& descriptorPool,
             Vulkan::DescriptorSetLayout& layout);

    auto getDescriptorSet(uint32_t index) -> Vulkan::DescriptorSet&;
    auto getPipeline() -> Vulkan::GraphicsPipeline*;
};