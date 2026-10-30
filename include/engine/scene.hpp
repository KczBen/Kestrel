#pragma once

#include "math.hpp"
#include "types.hpp"
#include <memory>

namespace Kestrel {
class Object;

class Scene {
  public:
    ~Scene();
    Scene(Scene&&) noexcept;
    Scene(const Scene&) = delete;
    auto operator=(Scene&&) noexcept -> Scene&;
    auto operator=(const Scene&) -> Scene& = delete;

    auto addObject(AssetHandle asset) -> Object;
    auto addCamera() -> void;

    auto getObjectLocation(ObjectHandle object) -> Vec3<float>;
    auto getObjectRotation(ObjectHandle object) -> Quat;
    auto getObjectScale(ObjectHandle object) -> Vec3<float>;
    auto getObjectTransform(ObjectHandle object) -> Transform;

    auto setObjectTransform(ObjectHandle object, Vec3<float> position, Quat rotation, Vec3<float> scale) -> void;
    auto setObjectTransform(ObjectHandle object, Transform transform) -> void;

  private:
    class Impl;
    explicit Scene(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl;

    friend class Engine;
};
} // namespace Kestrel
