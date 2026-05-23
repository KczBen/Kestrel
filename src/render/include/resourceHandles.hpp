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

template <typename T, typename Container = std::vector<T>> struct ResourceHandle {
  private:
    Container* backingStorage;
    size_t index;

  public:
    ResourceHandle(Container* backingStorage = nullptr, size_t index = 0)
        : backingStorage(backingStorage), index(index) {}

    auto operator*() const -> T& { return (*backingStorage)[index]; }
    auto operator->() const -> T* { return &(*backingStorage)[index]; }

    [[nodiscard]] auto isPresent() const -> bool { return backingStorage != nullptr; }
};
} // namespace Render