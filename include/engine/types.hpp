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

template <typename T> struct Handle {
    uint32_t index = 0xFFFFFFFF;
    uint32_t generation = 0;

    auto operator==(const Handle&) const -> bool = default;
};

using MaterialHandle = Handle<struct MaterialTag>;
using TextureHandle = Handle<struct TextureTag>;
using AssetHandle = Handle<struct AssetTag>;
using ObjectHandle = Handle<struct ObjectTag>;
using RenderTargetHandle = Handle<struct RenderTargetTag>;

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

template <typename Tag> struct std::hash<Kestrel::Handle<Tag>> {
    auto operator()(const Kestrel::Handle<Tag>& h) const noexcept -> size_t {
        return (size_t(h.generation) << 32) | h.index;
    }
};