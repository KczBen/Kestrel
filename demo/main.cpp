#include "engine/engine.hpp"
#include <print>

auto main() -> int {
    auto result = Kestrel::Engine::create({{1280, 720}});
    if (!result) {
        std::println(stderr, "Error: {}", result.error());
        return 1;
    }

    auto engine = *std::move(result);

    const auto scene = engine.createScene();

    engine.render();

    return 0;
}