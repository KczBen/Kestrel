#pragma once

#include "engine/types.hpp"
#include "util/hashHelpers.hpp"

#include "vulkan-wrapper/deviceBuffer.hpp"
#include <cassert>
#include <cstdint>

namespace Vulkan {
class Texture;
}

namespace Render {
struct MeshHandle {
    Vulkan::DeviceBuffer* vertexBuffer;
    Vulkan::DeviceBuffer* indexBuffer;

    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t indexOffset;
    uint32_t indexCount;
};
} // namespace Render

class Material;
class glTFObject;

template <typename T> class ResourceHandle;

template <typename T> class SlotMap {
    struct Slot {
        std::optional<T> value;
        uint32_t generation = 0;
    };

    size_t liveCount = 0;
    std::vector<Slot> slots;
    std::vector<uint32_t> freeList;

  public:
    SlotMap() = default;
    SlotMap(const SlotMap&) = delete;
    auto operator=(const SlotMap&) -> SlotMap& = delete;
    SlotMap(SlotMap&&) = delete;
    auto operator=(SlotMap&&) -> SlotMap& = delete;
    ~SlotMap() = default;

    auto insert(T value) -> ResourceHandle<T> {
        uint32_t idx = 0;
        if (!freeList.empty()) {
            idx = freeList.back();
            freeList.pop_back();
        } else {
            idx = static_cast<uint32_t>(slots.size());
            slots.emplace_back();
        }

        slots[idx].value.emplace(std::move(value));
        ++liveCount;
        return ResourceHandle<T>(this, idx, slots[idx].generation);
    }

    auto get(uint32_t index, uint32_t generation) -> T* {
        if (index >= slots.size()) {
            return nullptr;
        }

        auto& s = slots[index];
        return (s.value && s.generation == generation) ? &*s.value : nullptr;
    }

    auto makeHandle(uint32_t index, uint32_t generation) -> ResourceHandle<T> {
        return get(index, generation) ? ResourceHandle<T>(this, index, generation) : ResourceHandle<T>{};
    }

    void remove(const ResourceHandle<T>& handle) {
        if (!get(handle.index, handle.generation)) {
            return;
        }

        auto& s = slots[handle.index];
        s.value.reset();
        --liveCount;
        ++s.generation;
        freeList.push_back(handle.index);
    }

    [[nodiscard]] auto size() const -> size_t { return liveCount; }
    [[nodiscard]] auto empty() const -> bool { return liveCount == 0; }

    template <bool Const> class Iter {
        using SlotVec = std::conditional_t<Const, const std::vector<Slot>, std::vector<Slot>>;
        using Ref = std::conditional_t<Const, const T&, T&>;
        using Ptr = std::conditional_t<Const, const T*, T*>;

        SlotVec* slots;
        size_t i;

        void skipEmpty() {
            while (i < slots->size() && !(*slots)[i].value) {
                ++i;
            }
        }

      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = Ptr;
        using reference = Ref;

        Iter(SlotVec* s, size_t idx) : slots(s), i(idx) { skipEmpty(); }

        auto operator*() const -> Ref { return *(*slots)[i].value; }
        auto operator->() const -> Ptr { return &*(*slots)[i].value; }
        auto operator++() -> Iter& {
            ++i;
            skipEmpty();
            return *this;
        }
        auto operator++(int) -> Iter {
            Iter tmp = *this;
            ++*this;
            return tmp;
        }
        auto operator==(const Iter& o) const -> bool { return i == o.i; }
    };

    auto begin() { return Iter<false>(&slots, 0); }
    auto end() { return Iter<false>(&slots, slots.size()); }
    auto begin() const { return Iter<true>(&slots, 0); }
    auto end() const { return Iter<true>(&slots, slots.size()); }
};

template <typename T> struct PublicTag {
    static_assert(sizeof(T) == 0, "This resource type has no public handle; add a PublicTag specialization");
};
template <> struct PublicTag<Material> {
    using type = Kestrel::MaterialTag;
};
template <> struct PublicTag<Vulkan::Texture> {
    using type = struct Kestrel::TextureTag;
};
template <> struct PublicTag<glTFObject> {
    using type = struct Kestrel::AssetTag;
};

template <typename T> class ResourceHandle {
    SlotMap<T>* map = nullptr;
    uint32_t index = 0;
    uint32_t generation = 0;

    friend class SlotMap<T>;
    friend struct std::hash<ResourceHandle<T>>;

  public:
    ResourceHandle() = default;
    ResourceHandle(SlotMap<T>* m, uint32_t i, uint32_t g) : map(m), index(i), generation(g) {}

    auto operator*() const -> T& {
        T* p = map->get(index, generation);
        assert(p && "stale resource handle");
        return *p;
    }
    auto operator->() const -> T* { return &**this; }
    auto operator==(const ResourceHandle&) const -> bool = default;

    [[nodiscard]] auto isPresent() const -> bool { return map != nullptr; }
    [[nodiscard]] auto isValid() const -> bool { return map && map->get(index, generation); }
    [[nodiscard]] auto toPublic() const {
        using Tag = typename PublicTag<T>::type;
        return Kestrel::Handle<Tag>{index, generation};
    }
};

template <typename T> struct std::hash<ResourceHandle<T>> {
    auto operator()(const ResourceHandle<T>& handle) const noexcept -> std::size_t {
        std::size_t seed = 0;
        hashCombine(seed, std::hash<const void*>{}(handle.map));
        hashCombine(seed, std::hash<uint32_t>{}(handle.index));
        hashCombine(seed, std::hash<uint32_t>{}(handle.generation));
        return seed;
    }
};