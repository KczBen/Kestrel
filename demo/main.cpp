#include "engine/engine.hpp"
#include "engine/object.hpp"
#include <print>

auto main() -> int {
    auto result = Kestrel::Engine::create({{1280, 720}});
    if (!result) {
        std::println(stderr, "Error: {}", result.error());
        return 1;
    }

    auto engine = *std::move(result);

    auto scene = engine.createScene();

    auto asset = engine.importGLTF("models/CarConcept.glb");

    scene.addObject(asset);

    engine.render(scene);

    return 0;
}