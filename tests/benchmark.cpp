// path: tests/benchmark.cpp
// context: C++20 / minslab / benchmark
#include "minslab/slab_allocator.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <cstdlib>

struct Particle {
    float x, y, z;
    float vx, vy, vz;
    uint32_t id;
    uint32_t flags;
};

int main() {
    std::cout << "==============================================\n";
    std::cout << "  MinSlab Allocator vs std::malloc Benchmark  \n";
    std::cout << "==============================================\n\n";

    constexpr std::size_t ITERATIONS = 1'000'000;
    constexpr std::size_t BATCH_SIZE = 10'000;

    std::cout << "Testing with " << ITERATIONS << " small objects (" 
              << sizeof(Particle) << " bytes each):\n\n";

    // 1. MinSlab Benchmark
    {
        std::vector<Particle*> pointers(BATCH_SIZE, nullptr);
        auto start = std::chrono::high_resolution_clock::now();

        for (std::size_t iter = 0; iter < ITERATIONS / BATCH_SIZE; ++iter) {
            for (std::size_t i = 0; i < BATCH_SIZE; ++i) {
                pointers[i] = minslab::Allocator::instance().create<Particle>(
                    static_cast<float>(i), 0.0f, 0.0f,
                    0.1f, 0.2f, 0.3f,
                    static_cast<uint32_t>(i), 1u
                );
            }
            for (std::size_t i = 0; i < BATCH_SIZE; ++i) {
                minslab::Allocator::instance().destroy(pointers[i]);
            }
        }

        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        std::cout << "[MinSlab] Time: " << ms << " ms ("
                  << (ITERATIONS * 1000.0 / (ms > 0 ? ms : 1)) << " alloc+free/sec)\n";
    }

    // 2. std::malloc Benchmark
    {
        std::vector<Particle*> pointers(BATCH_SIZE, nullptr);
        auto start = std::chrono::high_resolution_clock::now();

        for (std::size_t iter = 0; iter < ITERATIONS / BATCH_SIZE; ++iter) {
            for (std::size_t i = 0; i < BATCH_SIZE; ++i) {
                void* mem = std::malloc(sizeof(Particle));
                pointers[i] = new (mem) Particle{
                    static_cast<float>(i), 0.0f, 0.0f,
                    0.1f, 0.2f, 0.3f,
                    static_cast<uint32_t>(i), 1u
                };
            }
            for (std::size_t i = 0; i < BATCH_SIZE; ++i) {
                pointers[i]->~Particle();
                std::free(pointers[i]);
            }
        }

        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        std::cout << "[std::malloc] Time: " << ms << " ms ("
                  << (ITERATIONS * 1000.0 / (ms > 0 ? ms : 1)) << " alloc+free/sec)\n";
    }

    std::cout << "\nBenchmark complete. Press Enter to exit...";
    std::cin.get();
    return 0;
}
