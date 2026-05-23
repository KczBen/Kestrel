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

template <typename T> struct ResourceHandle {
    std::vector<T>* backingStorage = nullptr;
    size_t index = 0;
    [[nodiscard]] auto get() const -> T* { return &(*backingStorage)[index]; }
};
} // namespace Render