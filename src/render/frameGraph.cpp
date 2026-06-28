#include "frameGraph.hpp"
#include "commandPool.hpp"
#include "pass.hpp"
#include "pipelineManager.hpp"
#include "vulkan/vulkan.hpp"

#include <print>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace Render {
FrameGraph::FrameGraph(Vulkan::PipelineManager& pipelineManager) : pipelineManager(&pipelineManager) {};

auto FrameGraph::addPass(std::unique_ptr<Pass> pass) -> PassHandle {
    if (!nameMap.contains(pass->name)) {
        for (auto output : pass->colorAttachments) {
            producers[*output].push_back(&*pass);

            // If the load op is eLoad we want to write to this pass as-is, so we're both providing and consuming it
            if (output.loadOp == vk::AttachmentLoadOp::eLoad) {
                consumers[*output].push_back(&*pass);
            }
        }

        for (auto input : pass->inputs) {
            consumers[*input].push_back(&*pass);
        }

        if (pass->depthInput.isPresent()) {
            consumers[*pass->depthInput].push_back(&*pass);
        }

        if (pass->depthOutput.isPresent()) {
            producers[*pass->depthOutput].push_back(&*pass);

            // same as with the colour targets
            if (pass->depthOutput.loadOp == vk::AttachmentLoadOp::eLoad) {
                consumers[*pass->depthOutput].push_back(&*pass);
            }
        }

        passes.push_back(std::move(pass));
        PassHandle handle{&passes, passes.size() - 1};
        // no idea why I need ->get() instead of just *handle here but oh well
        nameMap[handle->get()->name] = handle;

        return handle;
    }

    std::print("Frame graph: Warning: Pass with name {} already exists\n", pass->name);

    return nameMap.at(pass->name);
}

void FrameGraph::compile() {
    // Building edges
    std::unordered_map<Pass*, std::vector<Pass*>> edges;

    for (auto& pass : passes) {
        for (const auto& depString : pass->dependencies) {
            edges[&**nameMap[depString]].push_back(pass.get());
        }
    }

    auto reaches = [&](Pass* from, Pass* target) -> bool {
        std::unordered_set<Pass*> visited;
        std::stack<Pass*> stack;
        stack.push(from);
        while (!stack.empty()) {
            auto* current = stack.top();
            stack.pop();
            if (current == target) {
                return true;
            }
            if (visited.contains(current)) {
                continue;
            }
            visited.insert(current);
            for (auto* next : edges[current]) {
                stack.push(next);
            }
        }
        return false;
    };

    for (auto& [resource, consumerList] : consumers) {
        for (auto* producer : producers[resource]) {
            for (auto* consumer : consumerList) {
                if (producer != consumer && !reaches(consumer, producer)) {
                    edges[producer].push_back(consumer);
                }
            }
        }
    }

    // Sorting
    std::unordered_map<Pass*, int> inDegree;

    for (auto& pass : passes) {
        inDegree[pass.get()] = 0;
    }

    for (auto& [pass, dependents] : edges) {
        for (auto* dependent : dependents) {
            inDegree[dependent]++;
        }
    }

    std::queue<Pass*> queue;
    for (auto& [pass, degree] : inDegree) {
        if (degree == 0) {
            queue.push(pass);
        }
    }

    while (!queue.empty()) {
        auto* current = queue.front();
        queue.pop();
        sortedPasses.push_back(current);

        for (auto* dependent : edges[current]) {
            if (--inDegree[dependent] == 0) {
                queue.push(dependent);
            }
        }
    }

    if (sortedPasses.size() != passes.size()) {
        std::print("Frame graph: Error: Cycle detected in pass dependencies\n");
    }
}

void FrameGraph::execute(vk::CommandBuffer cmd) {
    for (auto* pass : sortedPasses) {
        std::vector<Vulkan::ImageTransitionInfo> transitions;

        transitions.reserve(pass->inputs.size() + pass->colorAttachments.size());
        for (auto colorAttachment : pass->colorAttachments) {
            transitions.emplace_back(
                Vulkan::ImageTransitionInfo{.image = &**colorAttachment,
                                            .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                            .dstStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                            .dstAccess = vk::AccessFlagBits2::eColorAttachmentWrite,
                                            .dstAspect = vk::ImageAspectFlagBits::eColor});
        }

        for (auto colorInput : pass->inputs) {
            transitions.emplace_back(
                Vulkan::ImageTransitionInfo{.image = &**colorInput,
                                            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                                            .dstStage = vk::PipelineStageFlagBits2::eFragmentShader,
                                            .dstAccess = vk::AccessFlagBits2::eShaderRead,
                                            .dstAspect = vk::ImageAspectFlagBits::eColor});
        }

        // Depth transitions, three mutually exclusive cases:
        // 1. depthOutput && depthInput on same handle: read+write access
        // 2. only depthOutput: write access only
        // 3. only depthInput: read-only access
        const auto& depthOut = pass->depthOutput;
        const auto& depthIn = pass->depthInput;
        const bool sameHandle = depthOut.isPresent() && depthIn.isPresent() && depthOut.handle == depthIn.handle;

        if (sameHandle) {
            transitions.emplace_back(
                Vulkan::ImageTransitionInfo{.image = &**depthOut,
                                            .newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                            .dstStage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                                        vk::PipelineStageFlagBits2::eLateFragmentTests,
                                            .dstAccess = vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                                                         vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                                            .dstAspect = vk::ImageAspectFlagBits::eDepth});
        } else if (depthOut.isPresent()) {
            transitions.emplace_back(
                Vulkan::ImageTransitionInfo{.image = &**depthOut,
                                            .newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                            .dstStage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                                        vk::PipelineStageFlagBits2::eLateFragmentTests,
                                            .dstAccess = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                                            .dstAspect = vk::ImageAspectFlagBits::eDepth});
        } else if (depthIn.isPresent()) {
            transitions.emplace_back(
                Vulkan::ImageTransitionInfo{.image = &**depthIn,
                                            .newLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal,
                                            .dstStage = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
                                            .dstAccess = vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                            .dstAspect = vk::ImageAspectFlagBits::eDepth});
        }

        Vulkan::CommandPool::transitionImages(transitions, cmd);

        std::vector<vk::RenderingAttachmentInfo> colorAttachmentInfos;

        for (const auto& attachment : pass->colorAttachments) {
            const auto& handle = attachment.handle;
            if (!handle.isPresent()) {
                continue;
            }

            const vk::ClearValue clearValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}};
            const vk::RenderingAttachmentInfo attachmentInfo(handle->getImageView(),
                                                             vk::ImageLayout::eColorAttachmentOptimal,
                                                             vk::ResolveModeFlagBits::eNone,
                                                             handle->getImageView(),
                                                             vk::ImageLayout::eColorAttachmentOptimal,
                                                             attachment.loadOp,
                                                             attachment.storeOp,
                                                             clearValue);

            colorAttachmentInfos.push_back(attachmentInfo);
        }

        std::optional<vk::RenderingAttachmentInfo> depthAttachmentInfo;
        const auto& depthHandle = pass->depthOutput.isPresent() ? pass->depthOutput : pass->depthInput;
        if (depthHandle.isPresent()) {
            const bool isReadOnly = !pass->depthOutput.isPresent();
            const auto layout = isReadOnly ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                           : vk::ImageLayout::eDepthStencilAttachmentOptimal;
            const vk::ClearValue clearValue = vk::ClearDepthStencilValue(1.0f, 0);
            depthAttachmentInfo = vk::RenderingAttachmentInfo(depthHandle.handle->getImageView(),
                                                              layout,
                                                              vk::ResolveModeFlagBits::eNone,
                                                              {},
                                                              vk::ImageLayout::eUndefined,
                                                              depthHandle.loadOp,
                                                              depthHandle.storeOp,
                                                              clearValue);
        }

        vk::Extent3D extent;
        for (const auto& attachment : pass->colorAttachments) {
            if (attachment.handle.isPresent()) {
                extent = attachment.handle->getExtent();
                break;
            }
        }
        if (extent.width == 0 && pass->depthOutput.isPresent()) {
            extent = pass->depthOutput.handle->getExtent();
        }

        const vk::Viewport viewport(
            0.0, 1.0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0, 1.0);

        cmd.setViewport(0, 1, &viewport);

        const vk::Rect2D scissor(vk::Offset2D(0), vk::Extent2D(extent.width, extent.height));
        cmd.setScissor(0, 1, &scissor);

        const vk::RenderingInfo renderInfo({},
                                           vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(extent.width, extent.height)),
                                           static_cast<uint32_t>(1),
                                           static_cast<uint32_t>(0),
                                           static_cast<uint32_t>(colorAttachmentInfos.size()),
                                           colorAttachmentInfos.data(),
                                           depthAttachmentInfo.has_value() ? &*depthAttachmentInfo : nullptr,
                                           nullptr);

        cmd.beginRendering(renderInfo);

        // Record pass
        pass->record({.pipelineManager = pipelineManager, .cmd = cmd});

        cmd.endRendering();
    }
}
} // namespace Render
