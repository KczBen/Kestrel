#pragma once

#include "engine/object.hpp"
#include "engine/scene.hpp"
#include "render/resourceManager.hpp"
#include "scene/scene.hpp"

#include <entt/entity/fwd.hpp>
#include <memory>

class Kestrel::Scene::Impl {
  private:
    Render::ResourceManager* resourceManager;
    entt::registry* registry;

    std::unique_ptr<::Scene::Scene> internalScene;

  public:
    Impl(Render::ResourceManager& resourceManager, entt::registry& registry)
        : resourceManager(&resourceManager), registry(&registry),
          internalScene(std::make_unique<::Scene::Scene>(::Scene::Scene(registry))) {}

    auto get() -> ::Scene::Scene& { return *internalScene; }

    auto addObject(AssetHandle asset) -> Object {
        auto internalHandle = resourceManager->resolve(asset);

        internalScene->addObject(internalHandle);

        return Object{};
    }
};