// path: src/slab_allocator.cpp
// context: C++20 / minslab / memory management
#include "minslab/slab_allocator.hpp"

#include <windows.h>
#include <system_error>
#include <new>
#include <cassert>

namespace minslab {

SlabPool::SlabPool(std::size_t object_size) : object_size_(object_size) {}

SlabPool::~SlabPool() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Free all active slabs
    SlabHeader* current = active_slab_;
    while (current) {
        SlabHeader* next = current->next_slab;
        VirtualFree(current, 0, MEM_RELEASE);
        current = next;
    }

    // Free all full slabs
    current = full_slabs_;
    while (current) {
        SlabHeader* next = current->next_slab;
        VirtualFree(current, 0, MEM_RELEASE);
        current = next;
    }

    active_slab_ = nullptr;
    full_slabs_ = nullptr;
}

void SlabPool::grow() {
    void* raw = VirtualAlloc(
        nullptr,
        SLAB_PAGE_SIZE,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!raw) {
        throw std::bad_alloc();
    }

    auto* header = static_cast<SlabHeader*>(raw);
    header->next_slab = active_slab_;
    header->object_size = object_size_;

    // Align start of payload right after SlabHeader
    std::size_t header_size = (sizeof(SlabHeader) + 15) & ~15;
    std::size_t available_bytes = SLAB_PAGE_SIZE - header_size;
    std::size_t capacity = available_bytes / object_size_;

    header->total_objects = capacity;
    header->free_objects = capacity;

    // Build intrusive free-list chain within unused chunks
    auto* current_block = reinterpret_cast<char*>(raw) + header_size;
    header->free_list = reinterpret_cast<FreeNode*>(current_block);

    for (std::size_t i = 0; i < capacity - 1; ++i) {
        auto* node = reinterpret_cast<FreeNode*>(current_block);
        current_block += object_size_;
        node->next = reinterpret_cast<FreeNode*>(current_block);
    }
    // Last node points to nullptr
    reinterpret_cast<FreeNode*>(current_block)->next = nullptr;

    active_slab_ = header;
}

void* SlabPool::allocate() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!active_slab_ || !active_slab_->free_list) {
        grow();
    }

    SlabHeader* slab = active_slab_;
    assert(slab != nullptr && slab->free_list != nullptr);

    // Pop head from free list
    FreeNode* node = slab->free_list;
    slab->free_list = node->next;
    slab->free_objects--;

    // If slab has no more free objects, move it to full_slabs_
    if (slab->free_objects == 0) {
        active_slab_ = slab->next_slab;
        slab->next_slab = full_slabs_;
        full_slabs_ = slab;
    }

    return static_cast<void*>(node);
}

void SlabPool::deallocate(void* ptr) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Find the enclosing 64KB slab chunk
    std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
    auto* slab = reinterpret_cast<SlabHeader*>(addr & ~(SLAB_PAGE_SIZE - 1));

    // Push freed block back onto intrusive free list
    auto* node = static_cast<FreeNode*>(ptr);
    node->next = slab->free_list;
    slab->free_list = node;
    slab->free_objects++;

    // If slab was completely full, it is in full_slabs_, move it back to active_slab_
    if (slab->free_objects == 1 && slab != active_slab_) {
        // Unlink from full_slabs_
        SlabHeader** curr = &full_slabs_;
        while (*curr && *curr != slab) {
            curr = &((*curr)->next_slab);
        }
        if (*curr) {
            *curr = slab->next_slab;
            slab->next_slab = active_slab_;
            active_slab_ = slab;
        }
    }
}

Allocator::Allocator()
    : pools_{
        SlabPool(16),
        SlabPool(32),
        SlabPool(64),
        SlabPool(128),
        SlabPool(256),
        SlabPool(512),
        SlabPool(1024),
        SlabPool(2048)
    } {}

Allocator::~Allocator() = default;

Allocator& Allocator::instance() {
    static Allocator inst;
    return inst;
}

int Allocator::size_to_class(std::size_t size) noexcept {
    for (std::size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
        if (size <= SIZE_CLASSES[i]) {
            return static_cast<int>(i);
        }
    }
    return -1; // Larger than max slab object size
}

void* Allocator::allocate(std::size_t bytes) {
    if (bytes == 0) return nullptr;

    int idx = size_to_class(bytes);
    if (idx >= 0) {
        return pools_[static_cast<std::size_t>(idx)].allocate();
    }

    // Direct VirtualAlloc for large requests
    void* ptr = VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void Allocator::deallocate(void* ptr, std::size_t bytes) {
    if (!ptr) return;

    int idx = size_to_class(bytes);
    if (idx >= 0) {
        pools_[static_cast<std::size_t>(idx)].deallocate(ptr);
        return;
    }

    VirtualFree(ptr, 0, MEM_RELEASE);
}

void* slab_alloc(std::size_t bytes) {
    return Allocator::instance().allocate(bytes);
}

void slab_free(void* ptr, std::size_t bytes) {
    Allocator::instance().deallocate(ptr, bytes);
}

} // namespace minslab
