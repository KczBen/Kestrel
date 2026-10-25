#pragma once

#include "types.hpp"

#include <string>
#include <variant>
#include <vector>

namespace Kestrel {
struct TextureBinding {
    std::string name;
    TextureHandle texture;
    int32_t UVIndex;
};

struct MaterialParameter {
    std::string name;
    std::variant<int32_t, uint32_t, float> value;
};

struct MaterialConfiguration {
    std::string vertexShaderCodePath;
    std::string fragmentShaderCodePath;

    std::vector<TextureBinding> textures;
    std::vector<MaterialParameter> materialParameters;
};
} // namespace Kestrel