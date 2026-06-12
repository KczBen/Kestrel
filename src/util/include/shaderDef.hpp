#pragma once

#include <glslang/Public/ShaderLang.h>
#include <string>

struct ShaderDef {
    std::string filePath;
    EShLanguage stage;

    auto operator==(const ShaderDef& other) const noexcept -> bool {
        return filePath == other.filePath && stage == other.stage;
    }
};

namespace std {
template <> struct hash<ShaderDef> {
    auto operator()(const ShaderDef& shader) const noexcept -> size_t {
        size_t seed = 0;
        hashCombine(seed, shader.filePath);
        hashCombine(seed, static_cast<int>(shader.stage));
        return seed;
    }

  private:
    template <typename T> static void hashCombine(size_t& seed, const T& value) noexcept {
        const std::hash<T> hasher;
        seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};
} // namespace std