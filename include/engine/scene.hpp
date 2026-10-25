#pragma once

#include "math.hpp"
#include "types.hpp"

namespace Scene {
class Scene;
} // namespace Scene

namespace Kestrel {
class Object;

class Scene {
  public:
    auto addObject(AssetHandle asset) -> Object;
    auto addCamera() -> void;

    auto getObjectLocation(ObjectHandle object) -> Vec3<float>;
    auto getObjectRotation(ObjectHandle object) -> Quat;
    auto getObjectScale(ObjectHandle object) -> Vec3<float>;
    auto getObjectTransform(ObjectHandle object) -> Transform;

    auto setObjectTransform(ObjectHandle object, Vec3<float> position, Quat rotation, Vec3<float> scale) -> void;
    auto setObjectTransform(ObjectHandle object, Transform transform) -> void;

  private:
    friend class Engine;
    Scene(::Scene::Scene* internalScene) : internalScene(internalScene) {};
    ::Scene::Scene* internalScene = nullptr;
};
} // namespace Kestrel
