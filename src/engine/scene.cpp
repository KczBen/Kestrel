#include "engine/scene.hpp"
#include "engine/object.hpp"
#include "engine/types.hpp"
#include "scene_impl.hpp"

namespace Kestrel {
Scene::~Scene() = default;
Scene::Scene(Scene&& other) noexcept : impl(std::move(other.impl)) {}
Scene::Scene(std::unique_ptr<Impl> impl) : impl(std::move(impl)) {}
auto Scene::addObject(AssetHandle asset) -> Object { return impl->addObject(asset); }
} // namespace Kestrel