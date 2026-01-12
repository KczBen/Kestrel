#pragma once

#include "shaderDef.hpp"
#include "vulkan/vulkan_raii.hpp"
#include <glslang/Public/ShaderLang.h>
#include <vector>

namespace Vulkan {
class ShaderModule {
  private:
    const vk::raii::Device* device;
    vk::raii::ShaderModule shaderModule = nullptr;

    static auto compileShader(const std::vector<char>& source, EShLanguage stage) -> std::vector<uint32_t>;

  public:
    ShaderModule(const vk::raii::Device* device, const ShaderDef& shader);
    auto get() -> const vk::raii::ShaderModule&;
};
} // namespace Vulkan