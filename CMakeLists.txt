cmake_minimum_required(VERSION 3.31.0)
project(vulkan-sdl VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")

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
    src/render/renderTarget.cpp
)

target_include_directories(vulkan-wrapper PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/vulkan/include
    ${CMAKE_SOURCE_DIR}/src/util/include
    ${CMAKE_SOURCE_DIR}/src/render/scene/include
    ${CMAKE_SOURCE_DIR}/src/render/include
    ${Stb_INCLUDE_DIR}

)

target_link_libraries(vulkan-wrapper PRIVATE
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
)

add_library(render
    src/render/scene.cpp
    src/render/render.cpp
    src/render/material.cpp
    src/render/geometryPass.cpp
    src/render/camera.cpp
    src/render/cameraController.cpp
    src/render/pass.cpp
    src/render/resourceManager.cpp
    src/render/scene/object.cpp
    src/render/cameraPass.cpp
)

target_include_directories(render PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/vulkan/include
    ${CMAKE_SOURCE_DIR}/src/util/include
    ${CMAKE_SOURCE_DIR}/src/render/scene/include
    ${CMAKE_SOURCE_DIR}/src/render/include
)

target_link_libraries(render PRIVATE
    glm::glm-header-only
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
)

add_executable(vulkan-sdl
    src/main.cpp
)

target_include_directories(vulkan-sdl PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/vulkan/include
    ${CMAKE_SOURCE_DIR}/src/util/include
    ${CMAKE_SOURCE_DIR}/src/render/include
    ${CMAKE_SOURCE_DIR}/src/render/scene/include
)

target_link_libraries(vulkan-sdl PRIVATE
    render
    vulkan-wrapper
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
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
        tests/renderTarget_unittest.cpp
        tests/uploadContext_unittest.cpp
        tests/material_unittest.cpp
        tests/camera_unittest.cpp
        tests/object_unittest.cpp
        tests/renderIntegration_unittest.cpp
    )
    target_include_directories(test-bin PRIVATE
        ${CMAKE_SOURCE_DIR}/src/vulkan/include
        ${CMAKE_SOURCE_DIR}/src/util/include
        ${CMAKE_SOURCE_DIR}/src/render/include
        ${CMAKE_SOURCE_DIR}/src/render/scene/include
    )
    target_link_libraries(test-bin
        render
        vulkan-wrapper
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

        add_test(NAME coverage_setup
            COMMAND ${CMAKE_COMMAND} -E true
        )
        set_tests_properties(coverage_setup PROPERTIES
            FIXTURES_SETUP coverage_run
        )

        add_test(NAME coverage_merge
            COMMAND bash -c "${LLVM_PROFDATA} merge -sparse ${CMAKE_BINARY_DIR}/coverage-*.profraw -o ${CMAKE_BINARY_DIR}/coverage.profdata"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        set_tests_properties(coverage_merge PROPERTIES
            FIXTURES_CLEANUP coverage_run
        )

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
        set_tests_properties(coverage_html PROPERTIES
            DEPENDS coverage_merge
            FIXTURES_CLEANUP coverage_run
        )
    else()
        gtest_discover_tests(test-bin)
    endif()
endif()