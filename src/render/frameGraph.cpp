#include "frameGraph.hpp"
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

    std::print("Frame graph: Warning: Pass with name {} already exists", pass->name);

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

void FrameGraph::execute(vk::CommandBuffer cmd) {}
} // namespace Render
