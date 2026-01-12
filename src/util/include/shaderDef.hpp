#pragma once

#include <glslang/Public/ShaderLang.h>
#include <string>

struct ShaderDef {
    std::string filePath;
    EShLanguage stage;
};