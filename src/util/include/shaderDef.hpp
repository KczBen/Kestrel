#pragma once

#include <glslang/Public/ShaderLang.h>
#include <string>

struct ShaderDef {
    std::string filePath;
    EShLanguage stage;
};

namespace std {
template <> struct hash<ShaderDef> {
    auto operator()(const ShaderDef& shader) const noexcept -> size_t {
        size_t seed = 0;
        hash_combine(seed, shader.filePath);
        hash_combine(seed, static_cast<int>(shader.stage));
        return seed;
    }

  private:
    template <typename T> static void hash_combine(size_t& seed, const T& value) noexcept {
        const std::hash<T> hasher;
        seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};
} // namespace std