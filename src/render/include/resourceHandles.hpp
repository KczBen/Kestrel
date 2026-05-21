#pragma once

#include "deviceBuffer.hpp"
#include <cstdint>
namespace Render {
struct MeshHandle {
    Vulkan::DeviceBuffer* vertexBuffer;
    Vulkan::DeviceBuffer* indexBuffer;

    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t indexOffset;
    uint32_t indexCount;
};

using MaterialHandle = uint32_t;
} // namespace Render