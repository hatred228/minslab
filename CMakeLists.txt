cmake_minimum_required(VERSION 3.20)
project(minslab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(minslab_core STATIC
    src/slab_allocator.cpp
)

target_include_directories(minslab_core PUBLIC include)

add_executable(bench_allocator tests/benchmark.cpp)
target_link_libraries(bench_allocator PRIVATE minslab_core)
