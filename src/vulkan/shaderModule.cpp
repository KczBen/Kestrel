#include "shaderModule.hpp"
#include "readfile.hpp"
#include <SDL3/SDL_filesystem.h>
#include <cstdint>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

Vulkan::ShaderModule::ShaderModule(const vk::raii::Device* device, const ShaderDef& shader) : device(device) {

    auto shaderSource = readFile(std::string(SDL_GetBasePath()).append(shader.filePath));
    auto compiledCode = compileShader(shaderSource, shader.stage);
    const vk::ShaderModuleCreateInfo shaderModuleCreateInfo(
        vk::ShaderModuleCreateFlags(), compiledCode.size() * sizeof(uint32_t), compiledCode.data());

    shaderModule = vk::raii::ShaderModule(*this->device, shaderModuleCreateInfo);
}

// This below was written by Claude LLM
auto Vulkan::ShaderModule::compileShader(const std::vector<char>& source, EShLanguage stage) -> std::vector<uint32_t> {
    // Initialize glslang if not already done
    static const bool glslangInitialized = []() -> bool { return glslang::InitializeProcess(); }();

    if (!glslangInitialized) {
        throw std::runtime_error("Failed to initialize glslang");
    }

    // Set up shader compilation
    const std::string sourceStr(source.begin(), source.end());
    const char* sourceCStr = sourceStr.c_str();

    glslang::TShader shader(stage);
    shader.setStrings(&sourceCStr, 1);

    // Configure for Vulkan 1.4 and SPIR-V 1.6
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 140);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_4);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    // Get default built-in resource limits
    const TBuiltInResource* resources = GetDefaultResources();

    // Parse the shader
    auto messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    // Set a default entry point name
    shader.setEntryPoint("main");
    shader.setSourceEntryPoint("main");

    if (!shader.parse(resources, 460, false, messages)) {
        std::string errorMsg = "Shader compilation failed:\n";
        errorMsg += shader.getInfoLog();
        errorMsg += "\n";
        errorMsg += shader.getInfoDebugLog();
        throw std::runtime_error(errorMsg);
    }

    // Link the shader into a program
    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages)) {
        std::string errorMsg = "Shader linking failed:\n";
        errorMsg += program.getInfoLog();
        errorMsg += "\n";
        errorMsg += program.getInfoDebugLog();
        throw std::runtime_error(errorMsg);
    }

    // Convert to SPIR-V
    std::vector<uint32_t> spirv;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    spvOptions.generateDebugInfo = false;
    spvOptions.disableOptimizer = false;
    spvOptions.optimizeSize = false;
    spvOptions.validate = true;

    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &logger, &spvOptions);

    // Check for any warnings or errors during SPIR-V generation
    const std::string logMessages = logger.getAllMessages();
    if (!logMessages.empty()) {
        // Log warnings but don't fail
        // In production, you might want to use a logging system here
    }

    return spirv;
}

auto Vulkan::ShaderModule::get() -> const vk::raii::ShaderModule& { return shaderModule; }