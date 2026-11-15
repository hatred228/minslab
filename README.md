# minslab

A high-throughput, low-fragmentation C++20 Slab Allocator for Windows designed for game engines and high-frequency systems.

## Overview

`minslab` implements the classic Bonwick Slab Allocation pattern with intrusive free lists. It solves memory fragmentation and locking overhead associated with standard heap allocators when managing millions of small, short-lived objects.

### Architecture Highlights

- **Segregated Size Classes**: Predefined power-of-two slab pools (16, 32, 64, 128, 256, 512, 1024, 2048 bytes).
- **Intrusive Free Lists**: Zero per-object memory overhead (`FreeNode*` is embedded directly into unused memory blocks).
- **O(1) Allocation & Deallocation**: Constant-time push/pop operations from the active slab's free list.
- **64KB Virtual Memory Chunks**: Direct memory reservation using `VirtualAlloc` aligned to system allocation granularity.
- **Automatic Chunk Promotion/Demotion**: Slabs automatically transition between `active` and `full` states as objects are allocated and freed.

## Memory Layout

```text
+--------------------------------------------------------------+
| SlabHeader (metadata, free_list head, object counts)         |
+--------------------------------------------------------------+
| Chunk 0 [FreeNode -> Chunk 1]                                |
+--------------------------------------------------------------+
| Chunk 1 [Allocated Payload]                                  |
+--------------------------------------------------------------+
| Chunk 2 [FreeNode -> Chunk 3]                                |
+--------------------------------------------------------------+
| ...                                                          |
+--------------------------------------------------------------+
```

## Quick Start (C++20)

```cpp
#include "minslab/slab_allocator.hpp"

struct Bullet {
    float x, y, z;
};

// Typed creation with forward arguments
Bullet* b = minslab::Allocator::instance().create<Bullet>(1.0f, 2.0f, 3.0f);

// Clean destruction and return to slab pool
minslab::Allocator::instance().destroy(b);
```

## Building & Benchmarking

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
.\Release\bench_allocator.exe
```

## License

MIT License
