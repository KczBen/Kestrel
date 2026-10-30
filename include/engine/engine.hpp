#pragma once

#include "material.hpp"
#include "scene.hpp"
#include "types.hpp"

#include <expected>
#include <filesystem>
#include <memory>

namespace Kestrel {
class Engine {
  public:
    ~Engine();
    Engine(Engine&&) noexcept;
    Engine(const Engine&) = delete;
    auto operator=(Engine&&) noexcept -> Engine&;
    auto operator=(const Engine&) -> Engine& = delete;

    static auto create(EngineConfiguration config) -> std::expected<Engine, EngineError>;
    auto importGLTF(const std::filesystem::path& path) -> AssetHandle;
    auto createMaterial(const MaterialConfiguration& config) -> MaterialHandle;
    auto createFrameGraph() -> FrameGraph;
    auto createScene() -> Scene;
    auto render(Scene& scene) -> void;

  private:
    class Impl;
    explicit Engine(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl;
};
} // namespace Kestrel