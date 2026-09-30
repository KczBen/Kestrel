#include "scene.hpp"
#include "camera.hpp"
#include "components.hpp"
#include "gltfAsset.hpp"
#include <entt/entity/fwd.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>

namespace Scene {
Scene::Scene(entt::registry& registry) : entityRegistry(&registry) {
    // just make *a* camera for now
    const auto defaultCamera = entityRegistry->create();
    entityRegistry->emplace<CameraSettings>(defaultCamera);
    entityRegistry->emplace<CameraTransform>(defaultCamera, glm::vec3{}, glm::quat{1.0f, 0.0f, 0.0f, 0.0f});
    entityRegistry->emplace<OrbitController>(defaultCamera);

    // Mark it active
    entityRegistry->emplace<ActiveCamera>(defaultCamera);

    Camera::initOrbitCamera(entityRegistry->get<CameraTransform>(defaultCamera),
                            entityRegistry->get<OrbitController>(defaultCamera));
};

auto Scene::getRegistry() -> entt::registry& { return *entityRegistry; }

void Scene::update(Input inputs, float deltaTime) {
    auto view = entityRegistry->view<CameraTransform, OrbitController, ActiveCamera>();

    for (auto [entity, transform, controller] : view.each()) {
        auto [newTransform, newController] = Camera::updateOrbitCamera(transform, controller, inputs, deltaTime);
        if (newTransform != transform) {
            entityRegistry->patch<CameraTransform>(entity, [&](auto& transform) -> void { transform = newTransform; });
        }
    }
}

void Scene::addObject(AssetHandle object) {
    auto buildNode = [&](this auto& self, glTFNode& nodeToLoad, std::optional<entt::entity> parent) -> void {
        const entt::entity currentNode = entityRegistry->create();

        entityRegistry->emplace<Node>(currentNode, nodeToLoad.name);
        entityRegistry->emplace<LocalTransform>(currentNode,
                                                LocalTransform{.rotation = nodeToLoad.rotation,
                                                               .position = nodeToLoad.translation,
                                                               .scale = nodeToLoad.scale});
        entityRegistry->emplace<Parent>(currentNode);
        entityRegistry->emplace<Children>(currentNode);

        for (auto& primitiveToLoad : nodeToLoad.primitives) {
            // We have a primitive
            // Now we need to put this in the registry
            const entt::entity uploadedPrimitive = entityRegistry->create();
            entityRegistry->emplace<Parent>(uploadedPrimitive, Parent{.value = currentNode});
            if (primitiveToLoad.uploadedMesh.has_value()) {
                entityRegistry->emplace<Primitive>(
                    uploadedPrimitive,
                    Primitive{.mesh = primitiveToLoad.uploadedMesh.value(), .material = primitiveToLoad.material});
            }

            else {
                entityRegistry->emplace<WantsPrimitive>(uploadedPrimitive,
                                                        WantsPrimitive{.primitive = &primitiveToLoad});
            }

            // Put ourselves in the Children list for the node
            (*entityRegistry->get<Children>(currentNode)).emplace_back(uploadedPrimitive);
        }

        // If we have a parent, put ourselves in the Children list too
        if (parent.has_value()) {
            (*entityRegistry->get<Children>(*parent)).emplace_back(currentNode);
        }

        // If we do not have one, make ourselves also an Object
        else {
            entityRegistry->emplace<Object>(currentNode);
        }

        for (auto& childNode : nodeToLoad.children) {
            self(childNode, currentNode);
        }
    };

    buildNode(object->root, std::nullopt);
}

void Scene::addLight(glm::vec3 position, glm::quat rotation, glm::vec3 colour, float intensity, float maxRange) {
    const entt::entity entity = entityRegistry->create();

    entityRegistry->emplace<LocalTransform>(entity, rotation, position, glm::vec3(1.0f));
    entityRegistry->emplace<LightSettings>(
        entity, LightSettings{.colour = colour, .intensity = intensity, .maxRange = maxRange});
}
} // namespace Scene