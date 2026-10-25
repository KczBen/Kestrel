#pragma once

#include "math.hpp"

#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

namespace Kestrel {
enum class EngineError : uint8_t { SDL_INIT_FAILED, NO_SUITABLE_GPU, INVALID_CONFIGURATION };

constexpr auto to_string(EngineError e) noexcept -> std::string_view { // NOLINT
    switch (e) {
    case EngineError::SDL_INIT_FAILED:
        return "SDL initialization failed";
    case EngineError::NO_SUITABLE_GPU:
        return "no suitable GPU found";
    case EngineError::INVALID_CONFIGURATION:
        return "invalid engine configuration";
    }
    std::unreachable();
}

struct MaterialHandle {
    uint32_t index, generation;
};
struct TextureHandle {
    uint32_t index, generation;
};
struct AssetHandle {
    uint32_t index, generation;
};
struct ObjectHandle {
    uint32_t index, generation;
};

struct EngineConfiguration {
    Vec2<uint32_t> windowSize;
};

struct Transform {
    Vec3<float> position{0, 0, 0};
    Quat rotation{.x = 0, .y = 0, .z = 0, .w = 1};
    Vec3<float> scale{1, 1, 1};
};

class FrameGraph {};

struct InputState {};
} // namespace Kestrel

template <> struct std::formatter<Kestrel::EngineError> : std::formatter<std::string_view> {
    auto format(Kestrel::EngineError e, auto& ctx) const {
        return std::formatter<std::string_view>::format(to_string(e), ctx);
    }
};