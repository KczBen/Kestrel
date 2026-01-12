#include "scene.hpp"
#include "camera.hpp"
#include "input.hpp"
#include "model.hpp"
#include <glm/fwd.hpp>
#include <memory>

Render::Scene::Scene(float cameraX, float cameraY, float cameraZ, float cameraFoV) {
    const glm::vec3 pos(cameraX, cameraY, cameraZ);
    camera = Camera(pos);
}

Render::Scene::Scene(Camera camera) : camera(camera) {};

void Render::Scene::addObject(std::unique_ptr<Model> model) { models.push_back(std::move(model)); }

void Render::Scene::update(Input input, float deltaTime) {
    for (auto& model : models) {
        model->update(deltaTime);
    }

    camera.update(input, deltaTime);
}

auto Render::Scene::getCamera() -> Camera& { return camera; }
auto Render::Scene::getObjects() const -> std::span<const Model* const> {
    static std::vector<const Model*> cache;
    cache.clear();

    for (const auto& m : models) {
        cache.push_back(m.get());
    }

    return cache;
}