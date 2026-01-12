cmake_minimum_required(VERSION 3.31.0)
project(vulkan-sdl VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED)
find_package(glslang REQUIRED)

add_executable(vulkan-sdl
    src/main.cpp

    src/util/model.cpp

    src/vulkan/instance.cpp
    src/vulkan/device.cpp
    src/vulkan/physicalDevice.cpp
    src/vulkan/surface.cpp
    src/vulkan/swapchain.cpp
    src/vulkan/image.cpp
    src/vulkan/texture.cpp
    src/vulkan/commandPool.cpp
    src/vulkan/descriptorPool.cpp
    src/vulkan/frameSync.cpp
    src/vulkan/semaphore.cpp
    src/vulkan/fence.cpp
    src/vulkan/shaderModule.cpp
    src/vulkan/graphicsPipeline.cpp
    src/vulkan/descriptorSetLayout.cpp
    src/vulkan/uploadBuffer.cpp
    src/vulkan/deviceBuffer.cpp
    src/vulkan/uploadContext.cpp
    src/vulkan/descriptorSet.cpp
    src/vulkan/hostBuffer.cpp
    
    src/render/scene.cpp
    src/render/render.cpp)

target_include_directories(vulkan-sdl PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/vulkan/include
    ${CMAKE_SOURCE_DIR}/src/util/include
    ${CMAKE_SOURCE_DIR}/src/render/include
)

target_link_libraries(vulkan-sdl PRIVATE
    Vulkan::Vulkan
    SDL3::SDL3
    glslang::glslang
    glslang::SPIRV
    glslang::glslang-default-resource-limits
)

add_custom_target(copy-compile-commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
)
