cmake_minimum_required(VERSION 3.31.0)
project(Kestrel VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")

add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
add_link_options(-fsanitize=address,undefined)

option(TEST_EXPORT_COVERAGE "Include coverage report when running tests" OFF)
option(TESTING "Build test targets" OFF)

if (TESTING)
    include(CTest)
    include(FetchContent)
    FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
    )

    FetchContent_MakeAvailable(googletest)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(glslang REQUIRED)
find_package(Stb REQUIRED)
find_package(EnTT CONFIG REQUIRED)

add_library(vulkan-wrapper
    src/vulkan-wrapper/instance.cpp
    src/vulkan-wrapper/device.cpp
    src/vulkan-wrapper/physicalDevice.cpp
    src/vulkan-wrapper/surface.cpp
    src/vulkan-wrapper/swapchain.cpp
    src/vulkan-wrapper/image.cpp
    src/vulkan-wrapper/sampler.cpp
    src/vulkan-wrapper/texture.cpp
    src/vulkan-wrapper/commandPool.cpp
    src/vulkan-wrapper/descriptorPool.cpp
    src/vulkan-wrapper/frameSync.cpp
    src/vulkan-wrapper/semaphore.cpp
    src/vulkan-wrapper/fence.cpp
    src/vulkan-wrapper/shaderModule.cpp
    src/vulkan-wrapper/graphicsPipeline.cpp
    src/vulkan-wrapper/descriptorSetLayout.cpp
    src/vulkan-wrapper/uploadBuffer.cpp
    src/vulkan-wrapper/deviceBuffer.cpp
    src/vulkan-wrapper/uploadContext.cpp
    src/vulkan-wrapper/descriptorSet.cpp
    src/vulkan-wrapper/pipelineManager.cpp
)

target_include_directories(vulkan-wrapper
    PUBLIC  ${CMAKE_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/src/util
            ${Stb_INCLUDE_DIR}
)
target_link_libraries(vulkan-wrapper PUBLIC Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV)

add_library(scene
    src/scene/scene.cpp
)
target_include_directories(scene
    PUBLIC  ${CMAKE_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/src/util
)
target_link_libraries(scene PUBLIC EnTT::EnTT glm::glm-header-only)

add_library(render
    src/render/render.cpp
    src/render/material.cpp
    src/render/geometryPass.cpp
    src/render/resourceManager.cpp
    src/render/cameraPass.cpp
    src/render/frameGraph.cpp
)

target_include_directories(render
    PUBLIC  ${CMAKE_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/src/util
            ${CMAKE_SOURCE_DIR}/thirdparty
)
target_link_libraries(render PUBLIC scene vulkan-wrapper glm::glm-header-only)

add_library(imgui
    thirdparty/imgui.cpp
    thirdparty/imgui_demo.cpp
    thirdparty/imgui_draw.cpp
    thirdparty/imgui_tables.cpp
    thirdparty/imgui_widgets.cpp
    thirdparty/imgui_impl_sdl3.cpp
    thirdparty/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty)
target_link_libraries(imgui PUBLIC SDL3::SDL3 Vulkan::Vulkan)

add_library(engine
    src/engine/engine.cpp
)

target_include_directories(engine
    PUBLIC  ${CMAKE_SOURCE_DIR}/include
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(engine
    PUBLIC scene render vulkan-wrapper imgui
    PRIVATE SDL3::SDL3 glslang::glslang
)

add_subdirectory(demo)

add_custom_target(copy-compile-commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
)

add_custom_target(pack-assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/models/
    ${CMAKE_BINARY_DIR}/models/

    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/textures/
    ${CMAKE_BINARY_DIR}/textures/

    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/src/shaders/
    ${CMAKE_BINARY_DIR}/shaders/
)

if(TESTING)
    enable_testing()
    add_executable(test-bin
        tests/vertex_unittest.cpp
        tests/device_unittest.cpp
        tests/instance_unittest.cpp
        tests/physicalDevice_unittest.cpp
        tests/commandPool_unittest.cpp
        tests/descriptorPool_unittest.cpp
        tests/descriptorSet_unittest.cpp
        tests/swapchain_unittest.cpp
        tests/image_unittest.cpp
        tests/fence_unittest.cpp
        tests/semaphore_unittest.cpp
        tests/deviceBuffer_unittest.cpp
        tests/uploadBuffer_unittest.cpp
        tests/hostBuffer_unittest.cpp
        tests/uploadContext_unittest.cpp
        tests/material_unittest.cpp
        tests/sampler_unittest.cpp
        tests/camera_unittest.cpp
        tests/firstPersonCamera_unittest.cpp
        tests/renderIntegration_unittest.cpp
        tests/renderTarget_unittest.cpp
        tests/frameGraph_unittest.cpp
    )
    target_link_libraries(test-bin PRIVATE
        scene
        render
        vulkan-wrapper
        imgui
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        SDL3::SDL3
        glslang::glslang
        glslang::SPIRV
        glslang::glslang-default-resource-limits
        GTest::gtest_main
    )
    include(GoogleTest)

    if(TEST_EXPORT_COVERAGE)
        target_compile_options(render PRIVATE -fprofile-instr-generate -fcoverage-mapping)
        target_compile_options(vulkan-wrapper PRIVATE -fprofile-instr-generate -fcoverage-mapping)
        target_compile_options(test-bin PRIVATE -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(test-bin PRIVATE -fprofile-instr-generate)

        find_program(LLVM_PROFDATA llvm-profdata REQUIRED)
        find_program(LLVM_COV llvm-cov REQUIRED)

        gtest_discover_tests(test-bin PROPERTIES
            ENVIRONMENT "LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/coverage-%p.profraw"
            FIXTURES_REQUIRED coverage_run
        )
        add_test(NAME coverage_setup COMMAND ${CMAKE_COMMAND} -E true)
        set_tests_properties(coverage_setup PROPERTIES FIXTURES_SETUP coverage_run)

        add_test(NAME coverage_merge
            COMMAND bash -c "${LLVM_PROFDATA} merge -sparse ${CMAKE_BINARY_DIR}/coverage-*.profraw -o ${CMAKE_BINARY_DIR}/coverage.profdata"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        set_tests_properties(coverage_merge PROPERTIES FIXTURES_CLEANUP coverage_run)

        add_test(NAME coverage_html
            COMMAND ${LLVM_COV} show $<TARGET_FILE:test-bin>
                -object $<TARGET_FILE:render>
                -object $<TARGET_FILE:vulkan-wrapper>
                -instr-profile=${CMAKE_BINARY_DIR}/coverage.profdata
                -format=html
                -output-dir=${CMAKE_BINARY_DIR}/coverage-html
                -ignore-filename-regex=.*/tests/.*
                ${CMAKE_SOURCE_DIR}/src
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        set_tests_properties(coverage_html PROPERTIES DEPENDS coverage_merge FIXTURES_CLEANUP coverage_run)
    else()
        gtest_discover_tests(test-bin)
    endif()
endif()