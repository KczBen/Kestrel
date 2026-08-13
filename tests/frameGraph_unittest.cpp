#include "camera.hpp"
#include "commandPool.hpp"
#include "device.hpp"
#include "frameGraph.hpp"
#include "instance.hpp"
#include "material.hpp"
#include "pass.hpp"
#include "physicalDevice.hpp"
#include "pipelineManager.hpp"
#include "renderTarget.hpp"
#include "resourceManager.hpp"
#include "surface.hpp"
#include <algorithm>
#include <entt/entt.hpp>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include <gtest/gtest.h>

namespace Render {

// Minimal test pass that does nothing in record()
class TestPass : public Pass {
  public:
    TestPass(std::string name,
             std::vector<RenderTargetAttachment> inputs,
             std::vector<RenderTargetAttachment> colorAttachments,
             RenderTargetAttachment depthInput,
             RenderTargetAttachment depthOutput,
             std::vector<std::string> dependencies)
        : Pass(std::move(name), inputs, colorAttachments, depthInput, depthOutput, std::move(dependencies)) {}

    void record(PassContext ctx) override {}
    [[nodiscard]] auto supportedMaterialTypes() const -> uint32_t override { return 0u; }
};

class FrameGraphFixture : public ::testing::Test {
  protected:
    void SetUp() override {
        instance_ = std::make_unique<Vulkan::Instance>("TestApp",
                                                       std::array<const char*, 3>{vk::KHRSurfaceExtensionName,
                                                                                  vk::EXTHeadlessSurfaceExtensionName,
                                                                                  VK_EXT_DEBUG_UTILS_EXTENSION_NAME});
        surface_ = std::make_unique<Vulkan::Surface>(*instance_);
        physicalDevice_ = std::make_unique<Vulkan::PhysicalDevice>(*instance_, *surface_);
        device_ = std::make_unique<Vulkan::Device>(*physicalDevice_, *instance_);
        entt::registry registry;
        resourceManager_ = std::make_unique<ResourceManager>(*physicalDevice_, *device_, registry);
        scene_ = std::make_unique<Scene::Scene>(registry);
        pipelineManager_ = &resourceManager_->getPipelineManager();
    }

    void TearDown() override { Test::TearDown(); }

    auto makeColorTarget() -> RenderTargetHandle {
        return resourceManager_->createRenderTarget("color_target",
                                                    vk::Extent3D{256, 256, 1},
                                                    vk::Format::eB8G8R8A8Unorm,
                                                    vk::ImageUsageFlagBits::eColorAttachment |
                                                        vk::ImageUsageFlagBits::eTransferSrc,
                                                    vk::ImageAspectFlagBits::eColor);
    }

    auto makeDepthTarget() -> RenderTargetHandle {
        return resourceManager_->createRenderTarget("depth_target",
                                                    vk::Extent3D{256, 256, 1},
                                                    vk::Format::eD32Sfloat,
                                                    vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                    vk::ImageAspectFlagBits::eDepth);
    }

    auto makePass(std::string name,
                  std::vector<RenderTargetAttachment> inputs = {},
                  std::vector<RenderTargetAttachment> colorAttachments = {},
                  RenderTargetAttachment depthInput = RenderTargetAttachment{},
                  RenderTargetAttachment depthOutput = RenderTargetAttachment{},
                  std::vector<std::string> dependencies = {}) -> std::unique_ptr<TestPass> {
        return std::make_unique<TestPass>(
            std::move(name), inputs, colorAttachments, depthInput, depthOutput, std::move(dependencies));
    }

  protected:
    std::unique_ptr<Vulkan::Instance> instance_;
    std::unique_ptr<Vulkan::Surface> surface_;
    std::unique_ptr<Vulkan::PhysicalDevice> physicalDevice_;
    std::unique_ptr<Vulkan::Device> device_;
    std::unique_ptr<ResourceManager> resourceManager_;
    std::unique_ptr<Scene::Scene> scene_;
    Vulkan::PipelineManager* pipelineManager_ = nullptr;
};

// Helper to find a pass by name in the sorted list
static auto findPassIndex(const std::vector<Pass*>& sorted, const std::string& name) -> size_t {
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (sorted[i]->getName() == name) {
            return i;
        }
    }
    return sorted.size();
}

TEST_F(FrameGraphFixture, CompileReturnsSortedPasses) {
    auto frameGraph = std::make_unique<FrameGraph>(*pipelineManager_);

    frameGraph->addPass(makePass("pass_a"));
    frameGraph->addPass(makePass("pass_b"));
    frameGraph->addPass(makePass("pass_c"));

    auto sorted = frameGraph->compile();
    ASSERT_EQ(sorted.size(), 3u);

    std::set<std::string> names;
    for (auto* pass : sorted) {
        names.insert(pass->getName());
    }
    ASSERT_EQ(names.count("pass_a"), 1u);
    ASSERT_EQ(names.count("pass_b"), 1u);
    ASSERT_EQ(names.count("pass_c"), 1u);
}

TEST_F(FrameGraphFixture, ExplicitDependencyOrdering) {
    auto frameGraph = std::make_unique<FrameGraph>(*pipelineManager_);

    frameGraph->addPass(makePass("pass_a"));
    frameGraph->addPass(makePass("pass_b", {}, {}, {}, {}, {"pass_a"}));

    auto sorted = frameGraph->compile();
    ASSERT_EQ(sorted.size(), 2u);

    auto idxA = findPassIndex(sorted, "pass_a");
    auto idxB = findPassIndex(sorted, "pass_b");

    EXPECT_LT(idxA, idxB) << "pass_a must come before pass_b due to explicit dependency";
}

TEST_F(FrameGraphFixture, ChainOfExplicitDependencies) {
    auto frameGraph = std::make_unique<FrameGraph>(*pipelineManager_);

    frameGraph->addPass(makePass("pass_a"));
    frameGraph->addPass(makePass("pass_b", {}, {}, {}, {}, {"pass_a"}));
    frameGraph->addPass(makePass("pass_c", {}, {}, {}, {}, {"pass_b"}));

    auto sorted = frameGraph->compile();
    ASSERT_EQ(sorted.size(), 3u);

    auto idxA = findPassIndex(sorted, "pass_a");
    auto idxB = findPassIndex(sorted, "pass_b");
    auto idxC = findPassIndex(sorted, "pass_c");

    EXPECT_LT(idxA, idxB) << "pass_a before pass_b";
    EXPECT_LT(idxB, idxC) << "pass_b before pass_c";
}

TEST_F(FrameGraphFixture, ImplicitResourceDependency) {
    auto frameGraph = std::make_unique<FrameGraph>(*pipelineManager_);

    auto colorTarget = makeColorTarget();

    // producer writes to colorTarget (output with clear/store)
    // consumer reads from colorTarget (input)
    // consumer must come after producer
    auto producerAtt = RenderTargetAttachment{colorTarget, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore};
    auto consumerInput = RenderTargetAttachment{colorTarget};

    frameGraph->addPass(makePass("producer", {}, {producerAtt}));
    frameGraph->addPass(makePass("consumer", {consumerInput}));

    auto sorted = frameGraph->compile();
    ASSERT_EQ(sorted.size(), 2u);

    auto idxProducer = findPassIndex(sorted, "producer");
    auto idxConsumer = findPassIndex(sorted, "consumer");

    EXPECT_LT(idxProducer, idxConsumer) << "producer must come before consumer due to implicit resource dependency";
}

TEST_F(FrameGraphFixture, ImplicitDepthDependency) {
    auto frameGraph = std::make_unique<FrameGraph>(*pipelineManager_);

    auto depthTarget = makeDepthTarget();

    auto depthOutput = RenderTargetAttachment{depthTarget, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore};
    auto depthInput = RenderTargetAttachment{depthTarget};

    // depth_pre writes depth (output), geometry reads depth (input)
    // depth_pre must come before geometry
    frameGraph->addPass(makePass("depth_pre", {}, {}, {}, depthOutput));
    frameGraph->addPass(makePass("geometry", {}, {}, depthInput));

    auto sorted = frameGraph->compile();
    ASSERT_EQ(sorted.size(), 2u);

    auto idxDepth = findPassIndex(sorted, "depth_pre");
    auto idxGeometry = findPassIndex(sorted, "geometry");

    EXPECT_LT(idxDepth, idxGeometry) << "depth_pre must come before geometry due to implicit depth dependency";
}

TEST_F(FrameGraphFixture, MixedExplicitAndImplicit) {
    auto frameGraph = std::make_unique<FrameGraph>(*pipelineManager_);

    auto colorTarget = makeColorTarget();
    auto depthTarget = makeDepthTarget();

    auto colorOut = RenderTargetAttachment{colorTarget, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore};
    auto colorIn = RenderTargetAttachment{colorTarget};
    auto depthOut = RenderTargetAttachment{depthTarget, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore};

    // pass_a: no resource deps
    // pass_b: produces colorTarget, explicit dep on pass_a
    // pass_c: consumes colorTarget, produces depthTarget, explicit dep on pass_b
    frameGraph->addPass(makePass("pass_a"));
    frameGraph->addPass(makePass("pass_b", {}, {colorOut}, {}, {}, {"pass_a"}));
    frameGraph->addPass(makePass("pass_c", {colorIn}, {}, {}, depthOut, {"pass_b"}));

    auto sorted = frameGraph->compile();
    ASSERT_EQ(sorted.size(), 3u);

    auto idxA = findPassIndex(sorted, "pass_a");
    auto idxB = findPassIndex(sorted, "pass_b");
    auto idxC = findPassIndex(sorted, "pass_c");

    EXPECT_LT(idxA, idxB) << "pass_a before pass_b (explicit dependency)";
    EXPECT_LT(idxB, idxC) << "pass_b before pass_c (explicit dependency)";
}

TEST_F(FrameGraphFixture, IndependentPassesAllPresent) {
    auto frameGraph = std::make_unique<FrameGraph>(*pipelineManager_);

    for (int i = 0; i < 5; ++i) {
        frameGraph->addPass(makePass("pass_" + std::to_string(i)));
    }

    auto sorted = frameGraph->compile();
    ASSERT_EQ(sorted.size(), 5u);

    for (int i = 0; i < 5; ++i) {
        std::string name = "pass_" + std::to_string(i);
        bool found = false;
        for (auto* pass : sorted) {
            if (pass->getName() == name) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "pass_" << i << " should be in sorted output";
    }
}

TEST_F(FrameGraphFixture, DiamondDependency) {
    auto frameGraph = std::make_unique<FrameGraph>(*pipelineManager_);

    // Diamond:
    //   pass_root
    //   /    \
    // pass_l  pass_r
    //   \    /
    //  pass_sink
    frameGraph->addPass(makePass("pass_root"));
    frameGraph->addPass(makePass("pass_l", {}, {}, {}, {}, {"pass_root"}));
    frameGraph->addPass(makePass("pass_r", {}, {}, {}, {}, {"pass_root"}));
    frameGraph->addPass(makePass("pass_sink", {}, {}, {}, {}, {"pass_l", "pass_r"}));

    auto sorted = frameGraph->compile();
    ASSERT_EQ(sorted.size(), 4u);

    auto idxRoot = findPassIndex(sorted, "pass_root");
    auto idxL = findPassIndex(sorted, "pass_l");
    auto idxR = findPassIndex(sorted, "pass_r");
    auto idxSink = findPassIndex(sorted, "pass_sink");

    EXPECT_LT(idxRoot, idxL) << "pass_root before pass_l";
    EXPECT_LT(idxRoot, idxR) << "pass_root before pass_r";
    EXPECT_LT(idxL, idxSink) << "pass_l before pass_sink";
    EXPECT_LT(idxR, idxSink) << "pass_r before pass_sink";
}

TEST_F(FrameGraphFixture, SinglePassNoDependencies) {
    auto frameGraph = std::make_unique<FrameGraph>(*pipelineManager_);

    frameGraph->addPass(makePass("only_pass"));

    auto sorted = frameGraph->compile();
    ASSERT_EQ(sorted.size(), 1u);
    EXPECT_EQ(sorted[0]->getName(), "only_pass");
}

TEST_F(FrameGraphFixture, RepeatedAddPassReturnsSameHandle) {
    auto frameGraph = std::make_unique<FrameGraph>(*pipelineManager_);

    auto handle1 = frameGraph->addPass(makePass("repeated"));
    auto handle2 = frameGraph->addPass(makePass("repeated"));

    EXPECT_EQ(handle1, handle2) << "Adding a pass with duplicate name returns the same handle";
}

} // namespace Render
