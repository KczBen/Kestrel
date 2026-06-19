#pragma once

#include "deviceBuffer.hpp"
#include "renderTarget.hpp"
#include <cassert>
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

    friend struct std::hash<ResourceHandle<T, Container>>;

  public:
    ResourceHandle(Container* backingStorage = nullptr, size_t index = 0)
        : backingStorage(backingStorage), index(index) {}

    auto operator*() const -> T& { return (*backingStorage)[index]; }
    auto operator->() const -> T* { return &(*backingStorage)[index]; }
    auto operator==(const ResourceHandle& other) const -> bool {
        return backingStorage == other.backingStorage && index == other.index;
    }

    [[nodiscard]] auto isPresent() const -> bool { return backingStorage != nullptr; }
};

} // namespace Render

template <typename T, typename Container> struct std::hash<Render::ResourceHandle<T, Container>> {
    auto operator()(const Render::ResourceHandle<T, Container>& handle) const -> size_t {
        return std::hash<size_t>{}(handle.index);
    }
};

using RenderTargetHandle = Render::ResourceHandle<Render::RenderTargetInfo>;
