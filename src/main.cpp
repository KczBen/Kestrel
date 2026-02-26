#include "input.hpp"
#include "model.hpp"
#include "render.hpp"
#include "scene.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <glslang/Public/ShaderLang.h>

#include <memory>
#include <print>
#include <vulkan/vulkan_core.h>

auto main(int /*unused*/, char** /*unused*/) -> int {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println(stderr, "Failed to initialize SDL: {}", SDL_GetError());
        return 1;
    }

    // Create window with Vulkan support
    SDL_Window* window = SDL_CreateWindow(
        "Vulkan prototype", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    SDL_SetWindowRelativeMouseMode(window, true);

    if (window == nullptr) {
        std::println(stderr, "Failed to create window: {}", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    // Vulkan resource scope
    {
        Render::Render renderer(window, 2);
        Render::Scene scene(5.0f, 0.0f, 5.0f, 90.0f);
        scene.addObject(std::make_unique<Model>(std::string(SDL_GetBasePath()).append("models/viking_room.obj")));

        renderer.uploadScene(scene);

        // Main loop
        bool running = true;
        SDL_Event event;

        // MAIN LOOP
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
            }

            input.updateKeys(SDL_GetKeyboardState(nullptr));

            auto currentTime = std::chrono::high_resolution_clock::now();
            const float time =
                std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();

            scene.update(input, time);
            renderer.render(scene, time);
            lastFrameTime = currentTime;
        }
    }

    // Cleanup
    glslang::FinalizeProcess();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
