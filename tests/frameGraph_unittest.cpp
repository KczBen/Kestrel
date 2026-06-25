#include "descriptorPool.hpp"
#include "device.hpp"
#include "frameGraph.hpp"
#include "instance.hpp"
#include "pass.hpp"
#include "physicalDevice.hpp"
#include "pipelineManager.hpp"
#include "resourceManager.hpp"
#include "surface.hpp"
#include <SDL3/SDL.h>
#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>

class FrameGraphFixture : public ::testing::Test {
  protected:
    void SetUp() override {
        std::array<const char*, 2> extensions = {vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName};

        instance_ = std::make_unique<Vulkan::Instance>("TestApp", extensions);
        surface_ = std::make_unique<Vulkan::Surface>(*instance_);
        physicalDevice_ = std::make_unique<Vulkan::PhysicalDevice>(*instance_, *surface_);
        device_ = std::make_unique<Vulkan::Device>(*physicalDevice_, *instance_);

        descriptorPool_ = std::make_unique<Vulkan::DescriptorPool>(*device_);
        pipelineManager_ = std::make_unique<Vulkan::PipelineManager>(*device_, *descriptorPool_);
        resourceManager_ = std::make_unique<Render::ResourceManager>(*physicalDevice_, *device_);
    }

    void TearDown() override { Test::TearDown(); }

    std::unique_ptr<Vulkan::Instance> instance_;
    std::unique_ptr<Vulkan::Surface> surface_;
    std::unique_ptr<Vulkan::PhysicalDevice> physicalDevice_;
    std::unique_ptr<Vulkan::Device> device_;

    std::unique_ptr<Vulkan::DescriptorPool> descriptorPool_;
    std::unique_ptr<Vulkan::PipelineManager> pipelineManager_;
    std::unique_ptr<Render::ResourceManager> resourceManager_;

  protected:
    class TestPass : public Render::Pass {
      public:
        TestPass(Vulkan::PipelineManager& pm,
                 const std::vector<Render::RenderTargetAttachment>& inputs,
                 const std::vector<Render::RenderTargetAttachment>& colorAttachments,
                 const Render::RenderTargetAttachment& depthInput,
                 const Render::RenderTargetAttachment& depthOutput)
            : Pass(pm, inputs, colorAttachments, depthInput, depthOutput) {}

        void record(vk::CommandBuffer, uint32_t) override {}
    };

    auto makeColorAttachment(Render::RenderTargetHandle handle) -> Render::RenderTargetAttachment {
        return Render::RenderTargetAttachment{handle, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore};
    }

    auto makeDepthAttachment(Render::RenderTargetHandle handle) -> Render::RenderTargetAttachment {
        return Render::RenderTargetAttachment{handle, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore};
    }

    auto makeEmptyDepthAttachment() -> Render::RenderTargetAttachment { return Render::RenderTargetAttachment{}; }

    auto makeTestPass(Vulkan::PipelineManager& pm,
                      std::vector<Render::RenderTargetHandle> targets,
                      std::vector<Render::RenderTargetHandle> inputs,
                      Render::RenderTargetHandle depthOutput,
                      Render::RenderTargetHandle depthInput) -> std::unique_ptr<TestPass> {
        std::vector<Render::RenderTargetAttachment> colorAttachments;
        for (auto& h : targets) {
            colorAttachments.push_back(makeColorAttachment(h));
        }
        std::vector<Render::RenderTargetAttachment> inputAttachments;
        for (auto& h : inputs) {
            inputAttachments.push_back(
                Render::RenderTargetAttachment{h, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare});
        }
        return std::make_unique<TestPass>(
            pm, inputAttachments, colorAttachments, makeDepthAttachment(depthInput), makeDepthAttachment(depthOutput));
    }

    auto makeTestPass(Vulkan::PipelineManager& pm,
                      std::vector<Render::RenderTargetHandle> targets,
                      std::vector<Render::RenderTargetHandle> inputs,
                      Render::RenderTargetHandle depthOutput) -> std::unique_ptr<TestPass> {
        std::vector<Render::RenderTargetAttachment> colorAttachments;
        for (auto& h : targets) {
            colorAttachments.push_back(makeColorAttachment(h));
        }
        std::vector<Render::RenderTargetAttachment> inputAttachments;
        for (auto& h : inputs) {
            inputAttachments.push_back(
                Render::RenderTargetAttachment{h, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare});
        }
        return std::make_unique<TestPass>(
            pm, inputAttachments, colorAttachments, makeEmptyDepthAttachment(), makeDepthAttachment(depthOutput));
    }

    auto makeTestPass(Vulkan::PipelineManager& pm,
                      std::vector<Render::RenderTargetHandle> targets,
                      std::vector<Render::RenderTargetHandle> inputs) -> std::unique_ptr<TestPass> {
        std::vector<Render::RenderTargetAttachment> colorAttachments;
        for (auto& h : targets) {
            colorAttachments.push_back(makeColorAttachment(h));
        }
        std::vector<Render::RenderTargetAttachment> inputAttachments;
        for (auto& h : inputs) {
            inputAttachments.push_back(
                Render::RenderTargetAttachment{h, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare});
        }
        return std::make_unique<TestPass>(
            pm, inputAttachments, colorAttachments, makeEmptyDepthAttachment(), makeEmptyDepthAttachment());
    }
};

TEST_F(FrameGraphFixture, DefaultConstruction) { Render::FrameGraph fg; }

TEST_F(FrameGraphFixture, AddPassStoresPass) {
    Render::FrameGraph fg;

    fg.addPass(makeTestPass(*pipelineManager_, {}, {}, {}));
    EXPECT_TRUE(true);
}

TEST_F(FrameGraphFixture, CompileEmpty) {
    Render::FrameGraph fg;
    auto order = fg.compile();
    EXPECT_TRUE(order.empty());
}

TEST_F(FrameGraphFixture, CompileSinglePass) {
    Render::FrameGraph fg;
    fg.addPass(makeTestPass(*pipelineManager_, {}, {}, {}));
    auto order = fg.compile();
    EXPECT_EQ(order.size(), 1);
    EXPECT_EQ(order[0], 0);
}

TEST_F(FrameGraphFixture, CompileIndependentPasses) {
    Render::FrameGraph fg;
    fg.addPass(makeTestPass(*pipelineManager_, {}, {}, {}));
    fg.addPass(makeTestPass(*pipelineManager_, {}, {}, {}));
    fg.addPass(makeTestPass(*pipelineManager_, {}, {}, {}));
    auto order = fg.compile();
    EXPECT_EQ(order.size(), 3);
    for (size_t i = 1; i < order.size(); ++i) {
        EXPECT_LT(order[i - 1], order[i]);
    }
}

TEST_F(FrameGraphFixture, CompileLinearDependency) {
    auto target0 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                        vk::Format::eB8G8R8A8Srgb,
                                                        vk::ImageUsageFlagBits::eColorAttachment |
                                                            vk::ImageUsageFlagBits::eInputAttachment,
                                                        vk::ImageAspectFlagBits::eColor);

    auto target1 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                        vk::Format::eB8G8R8A8Srgb,
                                                        vk::ImageUsageFlagBits::eColorAttachment |
                                                            vk::ImageUsageFlagBits::eInputAttachment,
                                                        vk::ImageAspectFlagBits::eColor);

    Render::FrameGraph fg;
    // Pass 0: produces target0
    fg.addPass(makeTestPass(*pipelineManager_, {target0}, {}));
    // Pass 1: consumes target0, produces target1
    fg.addPass(makeTestPass(*pipelineManager_, {target1}, {target0}));
    // Pass 2: consumes target1
    fg.addPass(makeTestPass(*pipelineManager_, {}, {target1}));

    auto order = fg.compile();
    EXPECT_EQ(order.size(), 3);
    size_t idx0 = 0, idx1 = 0, idx2 = 0;
    for (size_t i = 0; i < order.size(); ++i) {
        if (order[i] == 0)
            idx0 = i;
        if (order[i] == 1)
            idx1 = i;
        if (order[i] == 2)
            idx2 = i;
    }
    // Pass 0 must come before pass 1 (which consumes target0)
    EXPECT_LT(idx0, idx1);
    // Pass 1 must come before pass 2 (which consumes target1)
    EXPECT_LT(idx1, idx2);
}

TEST_F(FrameGraphFixture, CompileDiamondDependency) {
    auto target0 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                        vk::Format::eB8G8R8A8Srgb,
                                                        vk::ImageUsageFlagBits::eColorAttachment |
                                                            vk::ImageUsageFlagBits::eInputAttachment,
                                                        vk::ImageAspectFlagBits::eColor);

    auto target1 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                        vk::Format::eB8G8R8A8Srgb,
                                                        vk::ImageUsageFlagBits::eColorAttachment |
                                                            vk::ImageUsageFlagBits::eInputAttachment,
                                                        vk::ImageAspectFlagBits::eColor);

    Render::FrameGraph fg;
    // Pass 0: produces target0 only
    fg.addPass(makeTestPass(*pipelineManager_, {target0}, {}));
    // Pass 1: produces target1 only
    fg.addPass(makeTestPass(*pipelineManager_, {target1}, {}));
    // Pass 2: consumes both target0 and target1
    fg.addPass(makeTestPass(*pipelineManager_, {}, {target0, target1}));

    auto order = fg.compile();
    EXPECT_EQ(order.size(), 3);
    size_t idx0 = 0, idx1 = 0, idx2 = 0;
    for (size_t i = 0; i < order.size(); ++i) {
        if (order[i] == 0)
            idx0 = i;
        if (order[i] == 1)
            idx1 = i;
        if (order[i] == 2)
            idx2 = i;
    }
    // Both pass 0 and pass 1 must come before pass 2
    EXPECT_LT(idx0, idx2);
    EXPECT_LT(idx1, idx2);
}

TEST_F(FrameGraphFixture, CompileSelfConsumingPass) {
    auto target0 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                        vk::Format::eB8G8R8A8Srgb,
                                                        vk::ImageUsageFlagBits::eColorAttachment |
                                                            vk::ImageUsageFlagBits::eInputAttachment,
                                                        vk::ImageAspectFlagBits::eColor);

    Render::FrameGraph fg;
    // Pass 0: produces target0 and also consumes target0
    fg.addPass(makeTestPass(*pipelineManager_, {target0}, {target0}));
    auto order = fg.compile();
    EXPECT_EQ(order.size(), 1);
    EXPECT_EQ(order[0], 0);
}

TEST_F(FrameGraphFixture, CompileCyclicDependencyThrows) {
    auto target0 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                        vk::Format::eB8G8R8A8Srgb,
                                                        vk::ImageUsageFlagBits::eColorAttachment |
                                                            vk::ImageUsageFlagBits::eInputAttachment,
                                                        vk::ImageAspectFlagBits::eColor);

    auto target1 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                        vk::Format::eB8G8R8A8Srgb,
                                                        vk::ImageUsageFlagBits::eColorAttachment |
                                                            vk::ImageUsageFlagBits::eInputAttachment,
                                                        vk::ImageAspectFlagBits::eColor);

    Render::FrameGraph fg;
    // Pass 0 produces target0, consumes target1 -> depends on pass 1
    fg.addPass(makeTestPass(*pipelineManager_, {target0}, {target1}));
    // Pass 1 produces target1, consumes target0 -> depends on pass 0
    fg.addPass(makeTestPass(*pipelineManager_, {target1}, {target0}));

    EXPECT_THROW(fg.compile(), std::runtime_error);
}

TEST_F(FrameGraphFixture, CompileComplexDependencyChain) {
    auto t0 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                   vk::Format::eB8G8R8A8Srgb,
                                                   vk::ImageUsageFlagBits::eColorAttachment |
                                                       vk::ImageUsageFlagBits::eInputAttachment,
                                                   vk::ImageAspectFlagBits::eColor);
    auto t1 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                   vk::Format::eB8G8R8A8Srgb,
                                                   vk::ImageUsageFlagBits::eColorAttachment |
                                                       vk::ImageUsageFlagBits::eInputAttachment,
                                                   vk::ImageAspectFlagBits::eColor);
    auto t2 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                   vk::Format::eB8G8R8A8Srgb,
                                                   vk::ImageUsageFlagBits::eColorAttachment |
                                                       vk::ImageUsageFlagBits::eInputAttachment,
                                                   vk::ImageAspectFlagBits::eColor);
    auto t3 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                   vk::Format::eB8G8R8A8Srgb,
                                                   vk::ImageUsageFlagBits::eColorAttachment |
                                                       vk::ImageUsageFlagBits::eInputAttachment,
                                                   vk::ImageAspectFlagBits::eColor);

    Render::FrameGraph fg;
    // Pass 0: produces t0
    fg.addPass(makeTestPass(*pipelineManager_, {t0}, {}));
    // Pass 1: produces t1, consumes t0
    fg.addPass(makeTestPass(*pipelineManager_, {t1}, {t0}));
    // Pass 2: produces t2, consumes t1
    fg.addPass(makeTestPass(*pipelineManager_, {t2}, {t1}));
    // Pass 3: independent, produces t3
    fg.addPass(makeTestPass(*pipelineManager_, {t3}, {}));

    auto order = fg.compile();
    EXPECT_EQ(order.size(), 4);
    size_t idx0 = 0, idx1 = 0, idx2 = 0, idx3 = 0;
    for (size_t i = 0; i < order.size(); ++i) {
        if (order[i] == 0)
            idx0 = i;
        if (order[i] == 1)
            idx1 = i;
        if (order[i] == 2)
            idx2 = i;
        if (order[i] == 3)
            idx3 = i;
    }
    // Chain: 0 -> 1 -> 2
    EXPECT_LT(idx0, idx1);
    EXPECT_LT(idx1, idx2);
    (void)idx3;
}

TEST_F(FrameGraphFixture, CompileMultipleInputsSingleProducer) {
    auto target0 = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                        vk::Format::eB8G8R8A8Srgb,
                                                        vk::ImageUsageFlagBits::eColorAttachment |
                                                            vk::ImageUsageFlagBits::eInputAttachment,
                                                        vk::ImageAspectFlagBits::eColor);

    Render::FrameGraph fg;
    // Pass 0: produces target0
    fg.addPass(makeTestPass(*pipelineManager_, {target0}, {}));
    // Pass 1: consumes target0 twice
    fg.addPass(makeTestPass(*pipelineManager_, {}, {target0, target0}));
    // Pass 2: also consumes target0
    fg.addPass(makeTestPass(*pipelineManager_, {}, {target0}));

    auto order = fg.compile();
    EXPECT_EQ(order.size(), 3);
    size_t idx0 = 0, idx1 = 0, idx2 = 0;
    for (size_t i = 0; i < order.size(); ++i) {
        if (order[i] == 0)
            idx0 = i;
        if (order[i] == 1)
            idx1 = i;
        if (order[i] == 2)
            idx2 = i;
    }
    // Pass 0 must come before both 1 and 2
    EXPECT_LT(idx0, idx1);
    EXPECT_LT(idx0, idx2);
}

TEST_F(FrameGraphFixture, CompileDepthAttachmentDependency) {
    auto colorTarget = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                            vk::Format::eB8G8R8A8Srgb,
                                                            vk::ImageUsageFlagBits::eColorAttachment |
                                                                vk::ImageUsageFlagBits::eInputAttachment,
                                                            vk::ImageAspectFlagBits::eColor);

    auto depthTarget = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                            vk::Format::eD32Sfloat,
                                                            vk::ImageUsageFlagBits::eDepthStencilAttachment |
                                                                vk::ImageUsageFlagBits::eInputAttachment,
                                                            vk::ImageAspectFlagBits::eDepth);

    Render::FrameGraph fg;
    // Pass 0: produces colorTarget and depthTarget as depthAttachment
    fg.addPass(makeTestPass(*pipelineManager_, {colorTarget}, {}, depthTarget));
    // Pass 1: consumes colorTarget as input and depthTarget as depthAttachment (via inputs)
    fg.addPass(makeTestPass(*pipelineManager_, {}, {colorTarget, depthTarget}));

    auto order = fg.compile();
    EXPECT_EQ(order.size(), 2);
    size_t idx0 = 0, idx1 = 0;
    for (size_t i = 0; i < order.size(); ++i) {
        if (order[i] == 0)
            idx0 = i;
        if (order[i] == 1)
            idx1 = i;
    }
    // Pass 0 must come before pass 1 (which consumes both colorTarget and depthTarget)
    EXPECT_LT(idx0, idx1);
}

TEST_F(FrameGraphFixture, CompileDepthOutputToDepthInput) {
    auto depthTarget = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                            vk::Format::eD32Sfloat,
                                                            vk::ImageUsageFlagBits::eDepthStencilAttachment |
                                                                vk::ImageUsageFlagBits::eInputAttachment,
                                                            vk::ImageAspectFlagBits::eDepth);

    Render::FrameGraph fg;
    // Pass 0: produces depthTarget as depthAttachment
    fg.addPass(makeTestPass(*pipelineManager_, {}, {}, depthTarget));
    // Pass 1: consumes depthTarget as input (no regular color attachments)
    fg.addPass(makeTestPass(*pipelineManager_, {}, {depthTarget}));

    auto order = fg.compile();
    EXPECT_EQ(order.size(), 2);
    size_t idx0 = 0, idx1 = 0;
    for (size_t i = 0; i < order.size(); ++i) {
        if (order[i] == 0)
            idx0 = i;
        if (order[i] == 1)
            idx1 = i;
    }
    // Pass 0 must come before pass 1 (depthOutput -> depthInput dependency)
    EXPECT_LT(idx0, idx1);
}

TEST_F(FrameGraphFixture, CompileDepthOutputWithLoadCreatesDependency) {
    auto depthTarget = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                            vk::Format::eD32Sfloat,
                                                            vk::ImageUsageFlagBits::eDepthStencilAttachment |
                                                                vk::ImageUsageFlagBits::eInputAttachment,
                                                            vk::ImageAspectFlagBits::eDepth);

    Render::FrameGraph fg;
    // Pass 0: produces depthTarget with eClear (producer only)
    auto depthClear =
        Render::RenderTargetAttachment{depthTarget, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore};
    std::vector<Render::RenderTargetAttachment> emptyInputs;
    std::vector<Render::RenderTargetAttachment> emptyColorAttachments;
    fg.addPass(std::make_unique<TestPass>(
        *pipelineManager_, emptyInputs, emptyColorAttachments, makeEmptyDepthAttachment(), depthClear));

    // Pass 1: produces depthTarget with eLoad (consumer + producer)
    auto depthLoad =
        Render::RenderTargetAttachment{depthTarget, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore};
    fg.addPass(std::make_unique<TestPass>(
        *pipelineManager_, emptyInputs, emptyColorAttachments, makeEmptyDepthAttachment(), depthLoad));

    auto order = fg.compile();
    EXPECT_EQ(order.size(), 2);
    size_t idx0 = 0, idx1 = 0;
    for (size_t i = 0; i < order.size(); ++i) {
        if (order[i] == 0)
            idx0 = i;
        if (order[i] == 1)
            idx1 = i;
    }
    // Pass 0 must come before pass 1 (depthOutput with eLoad depends on the previous producer)
    EXPECT_LT(idx0, idx1);
}

TEST_F(FrameGraphFixture, CompileColorOutputWithLoadCreatesDependency) {
    auto colorTarget = resourceManager_->createRenderTarget(vk::Extent3D{800, 600, 1},
                                                            vk::Format::eB8G8R8A8Srgb,
                                                            vk::ImageUsageFlagBits::eColorAttachment |
                                                                vk::ImageUsageFlagBits::eInputAttachment,
                                                            vk::ImageAspectFlagBits::eColor);

    Render::FrameGraph fg;
    // Pass 0: produces colorTarget with eClear (producer only)
    Render::RenderTargetAttachment colorClear(colorTarget, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore);
    std::vector<Render::RenderTargetAttachment> emptyInputs;
    std::vector<Render::RenderTargetAttachment> colorAttachments0;
    colorAttachments0.push_back(colorClear);
    fg.addPass(std::make_unique<TestPass>(
        *pipelineManager_, emptyInputs, colorAttachments0, makeEmptyDepthAttachment(), makeEmptyDepthAttachment()));

    // Pass 1: produces colorTarget with eLoad (consumer + producer)
    Render::RenderTargetAttachment colorLoad(colorTarget, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore);
    std::vector<Render::RenderTargetAttachment> colorAttachments1;
    colorAttachments1.push_back(colorLoad);
    fg.addPass(std::make_unique<TestPass>(
        *pipelineManager_, emptyInputs, colorAttachments1, makeEmptyDepthAttachment(), makeEmptyDepthAttachment()));

    auto order = fg.compile();
    EXPECT_EQ(order.size(), 2);
    size_t idx0 = 0, idx1 = 0;
    for (size_t i = 0; i < order.size(); ++i) {
        if (order[i] == 0)
            idx0 = i;
        if (order[i] == 1)
            idx1 = i;
    }
    // Pass 0 must come before pass 1 (color output with eLoad depends on the previous producer)
    EXPECT_LT(idx0, idx1);
}
