#pragma once

#include "engine/engine.hpp"
#include "render/render.hpp"
#include "render/resourceManager.hpp"
#include "scene/components.hpp"
#include "scene/scene.hpp"
#include "util/input.hpp"
#include "vulkan-wrapper/device.hpp"
#include "vulkan-wrapper/instance.hpp"
#include "vulkan-wrapper/physicalDevice.hpp"
#include "vulkan-wrapper/surface.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <chrono>
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glslang/Public/ShaderLang.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <print>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

// Implementation for the actual Engine class, so this stays hidden from the end users
class Kestrel::Engine::Impl {
  public:
    auto init(const EngineConfiguration& config) -> std::expected<void, EngineError> {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::println("{}", SDL_GetError());
            return std::unexpected{EngineError::SDL_INIT_FAILED};
        }

        const auto width = static_cast<int32_t>(config.windowSize.x());
        const auto height = static_cast<int32_t>(config.windowSize.y());

        window = SDL_CreateWindow(
            "Kestrel", width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

        if (window == nullptr) {
            std::println("{}", SDL_GetError());
            SDL_Quit();
            return std::unexpected{EngineError::SDL_INIT_FAILED};
        }

        vulkanInstance = std::make_unique<Vulkan::Instance>("Kestrel", getInstanceExtensions());
        vulkanSurface = std::make_unique<Vulkan::Surface>(window, *vulkanInstance);
        vulkanPhysDevice = std::make_unique<Vulkan::PhysicalDevice>(*vulkanInstance, *vulkanSurface);
        vulkanDevice = std::make_unique<Vulkan::Device>(*vulkanPhysDevice, *vulkanInstance);
        vulkanResourceManager = std::make_unique<Render::ResourceManager>(*vulkanPhysDevice, *vulkanDevice, registry);

        renderer = std::make_unique<Render::Render>(
            window, *vulkanResourceManager, *vulkanInstance, *vulkanSurface, *vulkanPhysDevice, *vulkanDevice, 2);

        frameContextEntity = registry.create();
        registry.emplace<FrameContext>(frameContextEntity);

        initialised = true;

        return {};
    }

    auto createScene() -> Scene {
        currentScene = std::make_shared<::Scene::Scene>(registry);
        // TODO: return proper scene and not hard code any of this
        auto asset = vulkanResourceManager->importGLTF(std::string(SDL_GetBasePath()).append("models/CarConcept.glb"));

        currentScene->addObject(asset);
        currentScene->addLight({10.0, 10.0, 10.0}, {}, {255.0, 255.0, 255.0}, 0.5, 50.0);
        currentScene->addLight({0.0, 10.0, -10.0}, {}, {127.0, 0.0, 127.0}, 0.3, 50.0);

        return Scene{nullptr};
    }

    auto importGLTF(const std::filesystem::path& path) -> AssetHandle {
        auto handle = vulkanResourceManager->importGLTF(path.string());

        // TODO: return proper handle
        return {.index = 0, .generation = 0};
    }

    auto render() -> void {
        if (!initialised || !currentScene) {
            return;
        }

        renderer->init(*currentScene);

        bool running = true;
        SDL_Event event;

        // MAIN LOOP
        uint32_t currentFrame = 0;
        while (running) {
            // Handle events
            Input input;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }

                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                }

                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    input.updateMouse(event.motion);
                }

                if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                    input.updateScroll(event.wheel);
                }

                if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                    int32_t width = 0;
                    int32_t height = 0;
                    SDL_GetWindowSizeInPixels(window, &width, &height);
                }
            }

            input.updateKeys(SDL_GetKeyboardState(nullptr));

            const auto mouseButtons = SDL_GetMouseState(nullptr, nullptr);
            input.rightMouseButton = (mouseButtons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0;

            auto currentTime = std::chrono::high_resolution_clock::now();
            const float time =
                std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();

            currentScene->update(input, time);

            // Trigger UBO updates after we captured inputs and ran simulations
            const uint32_t frameIndex = (currentFrame++) % 2;
            registry.patch<FrameContext>(frameContextEntity, [&](FrameContext& fc) -> void { fc.index = frameIndex; });

            renderer->render(*currentScene, time);
            lastFrameTime = currentTime;
        }
    }

  private:
    SDL_Window* window;

    std::unique_ptr<Vulkan::Instance> vulkanInstance;
    std::unique_ptr<Vulkan::Surface> vulkanSurface;
    std::unique_ptr<Vulkan::PhysicalDevice> vulkanPhysDevice;
    std::unique_ptr<Vulkan::Device> vulkanDevice;
    std::unique_ptr<Render::ResourceManager> vulkanResourceManager;

    std::unique_ptr<Render::Render> renderer;

    std::shared_ptr<::Scene::Scene> currentScene;

    entt::registry registry;
    entt::entity frameContextEntity;

    std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<long, std::ratio<1, 1000000000>>>
        lastFrameTime = std::chrono::high_resolution_clock::now();

    bool initialised = false;

  public:
    static auto getInstanceExtensions() -> std::vector<const char*> {
        uint32_t count = 0;
        const auto* sdlExts = SDL_Vulkan_GetInstanceExtensions(&count);
        std::vector<const char*> extensions(sdlExts, sdlExts + count);
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
        return extensions;
    }
};