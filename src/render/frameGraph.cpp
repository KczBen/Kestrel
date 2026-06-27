#include "frameGraph.hpp"
#include "pipelineManager.hpp"

#include <algorithm>
#include <optional>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vulkan/vulkan.hpp>

namespace Render {
FrameGraph::FrameGraph(Vulkan::PipelineManager& pipelineManager) : pipelineManager(&pipelineManager) {};

void FrameGraph::addPass(std::unique_ptr<Pass> pass) { passes.push_back(std::move(pass)); }

auto FrameGraph::compile() -> std::vector<size_t> {
    const size_t n = passes.size();
    if (n == 0) {
        return {};
    }

    // Build the target map: (handle, producerIndex)
    using TargetProducer = std::pair<RenderTargetHandle, size_t>;
    std::vector<TargetProducer> targets;
    targets.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        for (size_t a = 0; a < passes[i]->getColorAttachmentCount(); ++a) {
            const auto& handle = passes[i]->getColorAttachment(a).handle;
            if (handle.isPresent()) {
                targets.emplace_back(handle, i);
            }
        }
        if (passes[i]->getDepthOutput().isPresent()) {
            targets.emplace_back(passes[i]->getDepthOutput().handle, i);
        }
    }

    // Find the producer for a given input handle
    // returns the last match so that when a pass itself wrote the same handle (eLoad vs eClear),
    // the actual producer pass is found instead of the current pass's own entry.
    auto findProducer = [&targets](const RenderTargetHandle& input) -> std::optional<size_t> {
        std::optional<size_t> result;
        for (const auto& [target, producerIndex] : targets) {
            if (target == input) {
                result = producerIndex;
            }
        }
        return result;
    };

    // Build adjacency list and in-degree count
    std::vector<std::vector<size_t>> adj(n);
    std::vector<int> inDegree(n, 0);

    for (size_t i = 0; i < n; ++i) {
        for (const auto& input : passes[i]->getInputs()) {
            if (!input.isPresent()) {
                continue;
            }
            auto maybeProducer = findProducer(input.handle);
            if (maybeProducer.has_value() && maybeProducer.value() != i) {
                const size_t producerIndex = maybeProducer.value();
                if (std::find(adj[producerIndex].begin(), adj[producerIndex].end(), i) == adj[producerIndex].end()) {
                    adj[producerIndex].push_back(i);
                    inDegree[i]++;
                }
            }
        }
        if (passes[i]->getDepthInput().isPresent()) {
            auto maybeProducer = findProducer(passes[i]->getDepthInput().handle);
            if (maybeProducer.has_value() && maybeProducer.value() != i) {
                const size_t producerIndex = maybeProducer.value();
                if (std::find(adj[producerIndex].begin(), adj[producerIndex].end(), i) == adj[producerIndex].end()) {
                    adj[producerIndex].push_back(i);
                    inDegree[i]++;
                }
            }
        }

        // Check color attachments with eLoad — consumer of previous producer
        for (size_t a = 0; a < passes[i]->getColorAttachmentCount(); ++a) {
            const auto& colorAtt = passes[i]->getColorAttachment(a);
            if (colorAtt.isPresent() && colorAtt.loadOp == vk::AttachmentLoadOp::eLoad) {
                auto colorProducer = findProducer(colorAtt.handle);
                if (colorProducer.has_value() && colorProducer.value() != i) {
                    const size_t producerIndex = colorProducer.value();
                    if (std::find(adj[producerIndex].begin(), adj[producerIndex].end(), i) ==
                        adj[producerIndex].end()) {
                        adj[producerIndex].push_back(i);
                        inDegree[i]++;
                    }
                }
            }
        }

        // Depth output with eLoad is a consumer of the previous producer
        const auto& depthOut = passes[i]->getDepthOutput();
        if (depthOut.isPresent() && depthOut.loadOp == vk::AttachmentLoadOp::eLoad) {
            auto depthProducer = findProducer(depthOut.handle);
            if (depthProducer.has_value() && depthProducer.value() != i) {
                const size_t producerIndex = depthProducer.value();
                if (std::find(adj[producerIndex].begin(), adj[producerIndex].end(), i) == adj[producerIndex].end()) {
                    adj[producerIndex].push_back(i);
                    inDegree[i]++;
                }
            }
        }
    }

    // Topological sort (Kahn's algorithm with priority queue for deterministic ordering)
    std::vector<size_t> order;
    order.reserve(n);
    std::priority_queue<size_t, std::vector<size_t>, std::greater<>> pq;

    for (size_t i = 0; i < n; ++i) {
        if (inDegree[i] == 0) {
            pq.push(i);
        }
    }

    while (!pq.empty()) {
        auto u = pq.top();
        pq.pop();
        order.push_back(u);

        for (auto v : adj[u]) {
            if (--inDegree[v] == 0) {
                pq.push(v);
            }
        }
    }

    if (order.size() != n) {
        throw std::runtime_error("FrameGraph: circular dependency detected among passes");
    }

    // Populate sortedPasses with explicit attachment info from passes
    sortedPasses.resize(n);
    sortedOrder = order;

    for (size_t sortedIdx = 0; sortedIdx < n; ++sortedIdx) {
        const size_t originalIdx = order[sortedIdx];
        auto& info = sortedPasses[sortedIdx];
        info.originalIndex = originalIdx;

        // Copy color attachments with their explicit load/store operations
        for (size_t a = 0; a < passes[originalIdx]->getColorAttachmentCount(); ++a) {
            info.colorAttachments.push_back(passes[originalIdx]->getColorAttachment(a));
        }

        info.inputs = passes[originalIdx]->getInputs();
        info.depthOutput = passes[originalIdx]->getDepthOutput();
        info.depthInput = passes[originalIdx]->getDepthInput();

        // Validate depthOutput and depthInput don't refer to different handles
        if (info.depthOutput.isPresent() && info.depthInput.isPresent() &&
            info.depthOutput.handle != info.depthInput.handle) {
            throw std::runtime_error(
                "FrameGraph: depthOutput and depthInput must refer to the same handle or one must be empty");
        }
    }

    return order;
}

void FrameGraph::execute(vk::CommandBuffer cmd, const std::vector<bool>& passEnabled) {
    if (sortedPasses.empty()) {
        return;
    }

    // Local image state map — fresh every frame, destroyed at end of execute()
    std::unordered_map<RenderTargetHandle, ImageState> imageStates;

    // Initialize imageStates for images used by enabled passes
    for (const auto& info : sortedPasses) {
        const bool enabled = passEnabled.empty() || passEnabled[info.originalIndex];
        if (!enabled) {
            continue;
        }

        for (const auto& attachment : info.colorAttachments) {
            if (!attachment.handle.isPresent()) {
                continue;
            }
            imageStates[attachment.handle] = {.layout = attachment.handle->layout,
                                              .stageMask = attachment.handle->stageMask,
                                              .accessFlags = attachment.handle->accessFlags};
        }
        if (info.depthOutput.isPresent()) {
            imageStates[info.depthOutput.handle] = {.layout = info.depthOutput.handle->layout,
                                                    .stageMask = info.depthOutput.handle->stageMask,
                                                    .accessFlags = info.depthOutput.handle->accessFlags};
        }
        if (info.depthInput.isPresent()) {
            imageStates[info.depthInput.handle] = {.layout = info.depthInput.handle->layout,
                                                   .stageMask = info.depthInput.handle->stageMask,
                                                   .accessFlags = info.depthInput.handle->accessFlags};
        }
        for (const auto& input : info.inputs) {
            if (!input.isPresent()) {
                continue;
            }
            imageStates[input.handle] = {.layout = input.handle->layout,
                                         .stageMask = input.handle->stageMask,
                                         .accessFlags = input.handle->accessFlags};
        }
    }

    // Process each pass in sorted order
    for (const auto& info : sortedPasses) {
        const bool enabled = passEnabled.empty() || passEnabled[info.originalIndex];
        if (!enabled) {
            continue;
        }

        // Collect barriers for this pass
        std::vector<vk::ImageMemoryBarrier2> barriers;

        // Color outputs
        for (const auto& attachment : info.colorAttachments) {
            const auto& handle = attachment.handle;
            if (!handle.isPresent()) {
                continue;
            }

            auto& state = imageStates[handle];
            if (state.layout == vk::ImageLayout::eColorAttachmentOptimal) {
                continue;
            }

            vk::ImageMemoryBarrier2 barrier{};
            barrier.srcStageMask = state.stageMask;
            barrier.srcAccessMask = state.accessFlags;
            barrier.oldLayout = state.layout;
            barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
            barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
            barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            barrier.image = static_cast<vk::Image>(handle->get());
            barrier.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
            barriers.push_back(barrier);

            state.layout = vk::ImageLayout::eColorAttachmentOptimal;
            state.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            state.accessFlags = vk::AccessFlagBits2::eColorAttachmentWrite;
        }

        // Color inputs
        for (const auto& input : info.inputs) {
            if (!input.isPresent()) {
                continue;
            }

            const auto& handle = input.handle;
            auto& state = imageStates[handle];
            if (state.layout != vk::ImageLayout::eShaderReadOnlyOptimal) {
                vk::ImageMemoryBarrier2 barrier{};
                barrier.srcStageMask = state.stageMask;
                barrier.srcAccessMask = state.accessFlags;
                barrier.oldLayout = state.layout;
                barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
                barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
                barrier.image = static_cast<vk::Image>(handle->get());
                barrier.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
                barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
                barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
                barriers.push_back(barrier);

                state.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
                state.stageMask = vk::PipelineStageFlagBits2::eFragmentShader;
                state.accessFlags = vk::AccessFlagBits2::eShaderRead;
            }
        }

        // Depth barriers — three mutually exclusive cases:
        // 1. depthOutput && depthInput on same handle → read+write access, single attachment
        // 2. only depthOutput → write access only
        // 3. only depthInput → read-only access, not bound as attachment
        {
            using depthTransition = struct {
                vk::ImageLayout layout;
                vk::PipelineStageFlags2 stageMask;
                vk::AccessFlags2 accessMask;
            };

            std::optional<depthTransition> transition;

            const auto& depthOut = info.depthOutput;
            const auto& depthIn = info.depthInput;
            const bool sameHandle = depthOut.isPresent() && depthIn.isPresent() && depthOut.handle == depthIn.handle;

            if (sameHandle) {
                transition = depthTransition{vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                             vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                                 vk::PipelineStageFlagBits2::eLateFragmentTests,
                                             vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                                                 vk::AccessFlagBits2::eDepthStencilAttachmentWrite};
            } else if (depthOut.isPresent()) {
                transition = depthTransition{vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                             vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                                 vk::PipelineStageFlagBits2::eLateFragmentTests,
                                             vk::AccessFlagBits2::eDepthStencilAttachmentWrite};
            } else if (depthIn.isPresent()) {
                transition = depthTransition{vk::ImageLayout::eDepthStencilReadOnlyOptimal,
                                             vk::PipelineStageFlagBits2::eEarlyFragmentTests,
                                             vk::AccessFlagBits2::eDepthStencilAttachmentRead};
            }

            if (transition && depthOut.isPresent()) {
                const auto& handle = depthOut.handle;
                auto& state = imageStates[handle];
                if (state.layout != transition->layout) {
                    vk::ImageMemoryBarrier2 barrier{};
                    barrier.srcStageMask = state.stageMask;
                    barrier.srcAccessMask = state.accessFlags;
                    barrier.oldLayout = state.layout;
                    barrier.newLayout = transition->layout;
                    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
                    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
                    barrier.image = static_cast<vk::Image>(handle->get());
                    barrier.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1};
                    barrier.dstStageMask = transition->stageMask;
                    barrier.dstAccessMask = transition->accessMask;
                    barriers.push_back(barrier);

                    state.layout = transition->layout;
                    state.stageMask = transition->stageMask;
                    state.accessFlags = transition->accessMask;
                }
            } else if (transition && depthIn.isPresent()) {
                const auto& handle = depthIn.handle;
                auto& state = imageStates[handle];
                if (state.layout != transition->layout) {
                    vk::ImageMemoryBarrier2 barrier{};
                    barrier.srcStageMask = state.stageMask;
                    barrier.srcAccessMask = state.accessFlags;
                    barrier.oldLayout = state.layout;
                    barrier.newLayout = transition->layout;
                    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
                    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
                    barrier.image = static_cast<vk::Image>(handle->get());
                    barrier.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1};
                    barrier.dstStageMask = transition->stageMask;
                    barrier.dstAccessMask = transition->accessMask;
                    barriers.push_back(barrier);

                    state.layout = transition->layout;
                    state.stageMask = transition->stageMask;
                    state.accessFlags = transition->accessMask;
                }
            }
        }

        // Emit barriers before beginRendering
        if (!barriers.empty()) {
            vk::DependencyInfo depInfo{};
            depInfo.setImageMemoryBarriers(barriers);
            cmd.pipelineBarrier2(depInfo);
        }

        // Set up render pass
        std::vector<vk::RenderingAttachmentInfo> colorAttachmentInfos;

        for (const auto& attachment : info.colorAttachments) {
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
        const auto& depthHandle = info.depthOutput.isPresent() ? info.depthOutput : info.depthInput;
        if (depthHandle.isPresent()) {
            const bool isReadOnly = !info.depthOutput.isPresent();
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

        // Get extent from first color attachment or depth attachment
        vk::Extent3D extent;
        for (const auto& attachment : info.colorAttachments) {
            if (attachment.handle.isPresent()) {
                extent = attachment.handle->getExtent();
                break;
            }
        }
        if (extent.width == 0 && info.depthOutput.isPresent()) {
            extent = info.depthOutput.handle->getExtent();
        }

        const vk::Viewport viewport(
            0.0, 0.0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0, 1.0);

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
        passes[info.originalIndex]->record({.pipelineManager = pipelineManager, .cmd = cmd});

        cmd.endRendering();

        // Update imageStates for post-write state and write back to Image objects
        for (const auto& attachment : info.colorAttachments) {
            const auto& handle = attachment.handle;
            if (!handle.isPresent()) {
                continue;
            }

            auto& state = imageStates[handle];
            state.layout = vk::ImageLayout::eColorAttachmentOptimal;
            state.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            state.accessFlags = vk::AccessFlagBits2::eColorAttachmentWrite;

            attachment.handle->layout = state.layout;
            attachment.handle->stageMask = state.stageMask;
            attachment.handle->accessFlags = state.accessFlags;
        }

        if (info.depthOutput.isPresent()) {
            const auto& handle = info.depthOutput.handle;
            auto& state = imageStates[handle];

            state.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            state.stageMask =
                vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
            state.accessFlags =
                vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;

            info.depthOutput.handle->layout = state.layout;
            info.depthOutput.handle->stageMask = state.stageMask;
            info.depthOutput.handle->accessFlags = state.accessFlags;
        }

        if (info.depthInput.isPresent() && info.depthInput.handle != info.depthOutput.handle) {
            const auto& handle = info.depthInput.handle;
            auto& state = imageStates[handle];

            state.layout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
            state.stageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
            state.accessFlags = vk::AccessFlagBits2::eDepthStencilAttachmentRead;

            info.depthInput.handle->layout = state.layout;
            info.depthInput.handle->stageMask = state.stageMask;
            info.depthInput.handle->accessFlags = state.accessFlags;
        }
    }
}
} // namespace Render
