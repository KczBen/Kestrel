#pragma once

#include "camera.hpp"
#include "cameraController.hpp"
#include "input.hpp"
#include "object.hpp"
#include <memory>
#include <span>
#include <string>

namespace Render {
class Scene {

  private:
    // Scene objects
    std::optional<CameraController> playerCamera;
    std::vector<Object> objects;

    std::vector<Camera> cameras;

    std::vector<std::string> modelPaths;

  public:
    Scene(const char* modelPath);
    void update(Input inputs, float deltaTime);
    void addObject(std::unique_ptr<Object> model);
    void addControlledCamera(CameraController cameraController);
    auto getCamera() -> Camera&;
    [[nodiscard]] auto getObjects() const -> std::span<const Object>;

    friend class ResourceManager;
};
} // namespace Render