#pragma once

#include "engine/types.hpp"
#include "types.hpp"

namespace Kestrel {
struct Camera {
    float FoV{};
    float nearPlane{};
    float farPlane{};
    Transform transform;
};

class FirstPersonController {
  public:
    void update(Camera& camera, const InputState& input, float dt);
    float moveSpeed = 5.0f;
    float lookSensitivity = 0.002f;

  private:
};
} // namespace Kestrel