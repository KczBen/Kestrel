#pragma once

#include "camera.hpp"
#include "input.hpp"
#include "model.hpp"
#include <memory>
#include <span>

namespace Render {
class Scene {
  private:
    // Scene objects
    Camera camera;
    // Lights?
    std::vector<std::unique_ptr<Model>> models;

  public:
    Scene();
    Scene(float cameraX, float cameraY, float cameraZ, float cameraFoV);
    Scene(Camera camera);
    void update(Input inputs, float deltaTime);
    void addObject(std::unique_ptr<Model> model);
    auto getCamera() -> Camera&;
    [[nodiscard]] auto getObjects() const -> std::span<const Model* const>;
};
} // namespace Render