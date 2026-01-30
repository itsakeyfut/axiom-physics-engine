#include "axiom/memory/heap_allocator.hpp"
#include "axiom/memory/stack_allocator.hpp"

#include <benchmark/benchmark.h>

#include <vector>

using namespace axiom::memory;

// ============================================================================
// Benchmark: StackAllocator vs HeapAllocator - LIFO pattern
// ============================================================================

static void BM_StackAllocator_LIFOAllocations(benchmark::State& state) {
    const size_t allocSize = 64;
    StackAllocator allocator(1024 * 1024);  // 1MB

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        // Allocate
        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(allocSize, 8);
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        // Deallocate in LIFO order
        for (int64_t i = state.range(0) - 1; i >= 0; --i) {
            allocator.deallocate(ptrs[static_cast<size_t>(i)], allocSize);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0) * 2);  // alloc + dealloc
    state.SetBytesProcessed(state.iterations() * state.range(0) * static_cast<int64_t>(allocSize));
}

static void BM_HeapAllocator_LIFOAllocations(benchmark::State& state) {
    const size_t allocSize = 64;
    HeapAllocator allocator;

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        // Allocate
        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(allocSize, 8);
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        // Deallocate in LIFO order
        for (int64_t i = state.range(0) - 1; i >= 0; --i) {
            allocator.deallocate(ptrs[static_cast<size_t>(i)], allocSize);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0) * 2);  // alloc + dealloc
    state.SetBytesProcessed(state.iterations() * state.range(0) * static_cast<int64_t>(allocSize));
}

// ============================================================================
// Benchmark: StackAllocator - Allocation patterns
// ============================================================================

static void BM_StackAllocator_SmallAllocations(benchmark::State& state) {
    const size_t allocSize = 16;
    StackAllocator allocator(1024 * 1024);  // 1MB

    for (auto _ : state) {
        allocator.reset();
        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(allocSize, 8);
            benchmark::DoNotOptimize(ptr);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
    state.SetBytesProcessed(state.iterations() * state.range(0) * static_cast<int64_t>(allocSize));
}

static void BM_StackAllocator_MediumAllocations(benchmark::State& state) {
    const size_t allocSize = 256;
    StackAllocator allocator(4 * 1024 * 1024);  // 4MB

    for (auto _ : state) {
        allocator.reset();
        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(allocSize, 8);
            benchmark::DoNotOptimize(ptr);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
    state.SetBytesProcessed(state.iterations() * state.range(0) * static_cast<int64_t>(allocSize));
}

static void BM_StackAllocator_LargeAllocations(benchmark::State& state) {
    const size_t allocSize = 4096;
    StackAllocator allocator(16 * 1024 * 1024);  // 16MB

    for (auto _ : state) {
        allocator.reset();
        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(allocSize, 8);
            benchmark::DoNotOptimize(ptr);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
    state.SetBytesProcessed(state.iterations() * state.range(0) * static_cast<int64_t>(allocSize));
}

// ============================================================================
// Benchmark: StackAllocator - Alignment overhead
// ============================================================================

static void BM_StackAllocator_Alignment8(benchmark::State& state) {
    StackAllocator allocator(1024 * 1024);

    for (auto _ : state) {
        allocator.reset();
        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(1, 8);
            benchmark::DoNotOptimize(ptr);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_StackAllocator_Alignment64(benchmark::State& state) {
    StackAllocator allocator(1024 * 1024);

    for (auto _ : state) {
        allocator.reset();
        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(1, 64);
            benchmark::DoNotOptimize(ptr);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// ============================================================================
// Benchmark: StackAllocator - Nested scopes (LIFO pattern)
// ============================================================================

static void BM_StackAllocator_NestedScopes(benchmark::State& state) {
    StackAllocator allocator(1024 * 1024);

    for (auto _ : state) {
        allocator.reset();
        for (int64_t i = 0; i < state.range(0); ++i) {
            // Simulate nested function calls with LIFO allocations
            void* ptr1 = allocator.allocate(64, 8);
            void* ptr2 = allocator.allocate(128, 8);
            void* ptr3 = allocator.allocate(256, 8);

            benchmark::DoNotOptimize(ptr1);
            benchmark::DoNotOptimize(ptr2);
            benchmark::DoNotOptimize(ptr3);

            // Deallocate in LIFO order
            allocator.deallocate(ptr3, 256);
            allocator.deallocate(ptr2, 128);
            allocator.deallocate(ptr1, 64);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0) * 3);
}

// ============================================================================
// Benchmark: StackArray helper
// ============================================================================

static void BM_StackArray_Creation(benchmark::State& state) {
    StackAllocator allocator(1024 * 1024);

    for (auto _ : state) {
        allocator.reset();
        for (int64_t i = 0; i < state.range(0); ++i) {
            StackArray<float> arr(allocator, 100);
            benchmark::DoNotOptimize(arr.data());
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// ============================================================================
// Register benchmarks
// ============================================================================

// LIFO pattern comparison
BENCHMARK(BM_StackAllocator_LIFOAllocations)->Range(10, 1000);
BENCHMARK(BM_HeapAllocator_LIFOAllocations)->Range(10, 1000);

// Allocation size variations
BENCHMARK(BM_StackAllocator_SmallAllocations)->Range(100, 10000);
BENCHMARK(BM_StackAllocator_MediumAllocations)->Range(100, 10000);
BENCHMARK(BM_StackAllocator_LargeAllocations)->Range(10, 1000);

// Alignment overhead
BENCHMARK(BM_StackAllocator_Alignment8)->Range(100, 10000);
BENCHMARK(BM_StackAllocator_Alignment64)->Range(100, 10000);

// Nested scopes (realistic physics solver pattern)
BENCHMARK(BM_StackAllocator_NestedScopes)->Range(10, 1000);

// StackArray helper
BENCHMARK(BM_StackArray_Creation)->Range(10, 1000);

BENCHMARK_MAIN();
