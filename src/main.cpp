#include "descriptorPool.hpp"
#include "device.hpp"
#include "input.hpp"
#include "instance.hpp"
#include "materialBrowser.hpp"
#include "objectBrowser.hpp"
#include "physicalDevice.hpp"
#include "render.hpp"
#include "resourceManager.hpp"
#include "scene.hpp"
#include "surface.hpp"

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
#include <entt/entity/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glslang/Public/ShaderLang.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "vulkan/vulkan.hpp"

#include <print>
#include <string>
#include <vulkan/vulkan_core.h>

static auto getInstanceExtensions() -> std::vector<const char*> {
    uint32_t count = 0;
    const auto* sdlExts = SDL_Vulkan_GetInstanceExtensions(&count);
    std::vector<const char*> extensions(sdlExts, sdlExts + count);
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
    return extensions;
}

auto main(int /*unused*/, char** /*unused*/) -> int {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println(stderr, "Failed to initialize SDL: {}", SDL_GetError());
        return 1;
    }

    // Create window with Vulkan support
    SDL_Window* window = SDL_CreateWindow(
        "Vulkan prototype", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    // SDL_SetWindowRelativeMouseMode(window, true);

    if (window == nullptr) {
        std::println(stderr, "Failed to create window: {}", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Initialize ImGui
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(window);

    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    // Vulkan resource scope
    {
        Vulkan::Instance instance("Vulkan prototype", getInstanceExtensions());
        Vulkan::Surface surface(window, instance);
        Vulkan::PhysicalDevice physicalDevice(instance, surface);
        Vulkan::Device device(physicalDevice, instance);

        entt::registry registry;

        Render::ResourceManager resourceManager(physicalDevice, device, registry);
        Scene::Scene scene = (registry);
        resourceManager.loadScene(scene);

        auto asset = resourceManager.importGLTF(std::string(SDL_GetBasePath()).append("models/CarConcept.glb"));

        scene.addObject(asset);

        Render::Render renderer(window, resourceManager, instance, surface, physicalDevice, device, 2);

        renderer.init(scene);

        const Vulkan::DescriptorPool imguiDescriptorPool(device);

        const VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = **instance;
        initInfo.PhysicalDevice = **physicalDevice;
        initInfo.Device = **device;
        initInfo.QueueFamily = *physicalDevice.getQueueFamilyIndices().graphicsFamily;
        initInfo.Queue = *device.getGraphicsQueue();
        initInfo.DescriptorPool = **imguiDescriptorPool;
        initInfo.MinImageCount = 2;
        initInfo.ImageCount = 2;
        initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {.sType =
                                                                     VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                                                                 .colorAttachmentCount = 1,
                                                                 .pColorAttachmentFormats = &colorFormat};
        initInfo.UseDynamicRendering = true;
        ImGui_ImplVulkan_Init(&initInfo);

        // Main loop
        bool running = true;
        SDL_Event event;

        // MAIN LOOP
        while (running) {
            // Handle events
            Input input;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL3_ProcessEvent(&event);
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

                if (event.type == SDL_EVENT_MOUSE_WHEEL && !ImGui::GetIO().WantCaptureMouse) {
                    input.updateScroll(event.wheel);
                }

                if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                    int32_t width = 0;
                    int32_t height = 0;
                    SDL_GetWindowSizeInPixels(window, &width, &height);
                    // if (height > 0) {
                    //     scene.getCameras().data()->setAspectRatio(static_cast<float>(width) / height);
                    // }
                }
            }

            input.updateKeys(SDL_GetKeyboardState(nullptr));

            const auto mouseButtons = SDL_GetMouseState(nullptr, nullptr);
            input.rightMouseButton = (mouseButtons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0;

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            Render::MaterialBrowser materialBrowser(resourceManager);
            materialBrowser.update();

            Render::ObjectBrowser objectBrowser(scene, resourceManager.getMaterials());
            objectBrowser.update();

            auto currentTime = std::chrono::high_resolution_clock::now();
            const float time =
                std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();

            scene.update(input, time);
            renderer.render(scene, time);
            lastFrameTime = currentTime;
        }

        // kind of hacky, shouldn't be here ideally
        device->waitIdle();
        ImGui_ImplVulkan_Shutdown();
    }

    // Cleanup
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    glslang::FinalizeProcess();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
