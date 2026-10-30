#pragma once

#include "math.hpp"
#include "scene.hpp"
#include "types.hpp"

namespace Kestrel {
class Object {
  public:
    auto rotate(Kestrel::Quat deltaAngle) -> void {
        auto transform = scene->getObjectTransform(handle);

        scene->setObjectTransform(handle, transform);
    };

    auto setOrientation(Kestrel::Quat rotation) -> void;

  private:
    friend class Scene;
    Scene* scene = nullptr;
    Kestrel::ObjectHandle handle;
};
} // namespace Kestrel