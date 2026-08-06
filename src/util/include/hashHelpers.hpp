#pragma once

#include "glm/ext/vector_float2.hpp"
#include <cstddef>
#include <functional>

// Hash helpers and some hash functions for external types

template <typename T> void hashCombine(std::size_t& seed, const T& value) noexcept {
    const std::hash<T> hasher;
    seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <> struct std::hash<glm::vec2> {
    auto operator()(const glm::vec2& v) const noexcept -> size_t {
        size_t seed = 0;
        hashCombine(seed, v.x);
        hashCombine(seed, v.y);
        return seed;
    }
};