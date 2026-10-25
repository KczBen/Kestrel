#include "engine/engine.hpp"
#include "engine_impl.hpp"

namespace Kestrel {
Engine::~Engine() = default;
Engine::Engine(Engine&& other) noexcept : impl(std::move(other.impl)) {}
Engine::Engine(std::unique_ptr<Impl> impl) : impl(std::move(impl)) {}

auto Engine::create(EngineConfiguration config) -> std::expected<Engine, EngineError> {
    auto impl = std::make_unique<Engine::Impl>();
    auto result = impl->init(config);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }

    return Engine{std::move(impl)};
}

auto Engine::createScene() -> Scene { return impl->createScene(); }
auto Engine::render() -> void { impl->render(); }
} // namespace Kestrel