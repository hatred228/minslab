// path: include/minslab/slab_allocator.hpp
// context: C++20 / minslab / memory management
#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <array>

namespace minslab {

// Size classes for sub-allocations
inline constexpr std::size_t NUM_SIZE_CLASSES = 8;
inline constexpr std::array<std::size_t, NUM_SIZE_CLASSES> SIZE_CLASSES = {
    16, 32, 64, 128, 256, 512, 1024, 2048
};
inline constexpr std::size_t MAX_SLAB_OBJECT_SIZE = 2048;
inline constexpr std::size_t SLAB_PAGE_SIZE = 64 * 1024; // 64 KB per slab chunk

struct FreeNode {
    FreeNode* next{nullptr};
};

struct SlabHeader {
    SlabHeader* next_slab{nullptr};
    std::size_t class_index{0};
    std::size_t object_size{0};
    std::size_t total_objects{0};
    std::size_t free_objects{0};
    FreeNode* free_list{nullptr};
};

class SlabPool {
public:
    explicit SlabPool(std::size_t object_size);
    ~SlabPool();

    SlabPool(const SlabPool&) = delete;
    SlabPool& operator=(const SlabPool&) = delete;

    void* allocate();
    void deallocate(void* ptr);

    [[nodiscard]] std::size_t object_size() const noexcept { return object_size_; }

private:
    void grow();

    std::size_t object_size_;
    SlabHeader* active_slab_{nullptr};
    SlabHeader* full_slabs_{nullptr};
    std::mutex mutex_;
};

class Allocator {
public:
    Allocator();
    ~Allocator();

    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;

    static Allocator& instance();

    void* allocate(std::size_t bytes);
    void deallocate(void* ptr, std::size_t bytes);

    template <typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    template <typename T>
    void destroy(T* ptr) {
        if (!ptr) return;
        ptr->~T();
        deallocate(ptr, sizeof(T));
    }

private:
    [[nodiscard]] static int size_to_class(std::size_t size) noexcept;

    std::array<SlabPool, NUM_SIZE_CLASSES> pools_;
};

// Global convenience wrappers
void* slab_alloc(std::size_t bytes);
void slab_free(void* ptr, std::size_t bytes);

} // namespace minslab
