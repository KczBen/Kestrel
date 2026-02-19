cmake_minimum_required(VERSION 3.31.0)
project(vulkan-sdl VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
)

FetchContent_MakeAvailable(googletest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TEST_EXPORT_COVERAGE "Include coverage report when running tests" OFF)

find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(glslang REQUIRED)

add_library(vulkan-wrapper
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
    src/vulkan/pipelineManager.cpp
)

target_include_directories(vulkan-wrapper PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/vulkan/include
    ${CMAKE_SOURCE_DIR}/src/util/include
    ${CMAKE_SOURCE_DIR}/src/render/include
)

add_library(util
    src/util/model.cpp
)

target_include_directories(util PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/util/include
)

add_library(render
    src/render/scene.cpp
    src/render/render.cpp
    src/render/material.cpp
)

target_include_directories(render PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/vulkan/include
    ${CMAKE_SOURCE_DIR}/src/util/include
    ${CMAKE_SOURCE_DIR}/src/render/include
)

add_executable(vulkan-sdl
    src/main.cpp
)

target_include_directories(vulkan-sdl PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/vulkan/include
    ${CMAKE_SOURCE_DIR}/src/util/include
    ${CMAKE_SOURCE_DIR}/src/render/include
)

target_link_libraries(vulkan-sdl PRIVATE
    render
    vulkan-wrapper
    util
    glm::glm-header-only
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    SDL3::SDL3
    glslang::glslang
    glslang::SPIRV
    glslang::glslang-default-resource-limits
)

enable_testing()
add_executable(vertex_test
    tests/vertex_unittest.cpp
)

target_include_directories(vertex_test PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/vulkan/include
    ${CMAKE_SOURCE_DIR}/src/util/include
    ${CMAKE_SOURCE_DIR}/src/render/include
)

target_link_libraries(vertex_test
    render
    vulkan-wrapper
    util
    GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(vertex_test)

add_custom_target(copy-compile-commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
)

if(TEST_EXPORT_COVERAGE)
    # Build with coverage info
    target_compile_options(render vulkan-wrapper util PRIVATE -coverage)
    target_link_options(render vulkan-wrapper util PRIVATE -coverage)

endif()