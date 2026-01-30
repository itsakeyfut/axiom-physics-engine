#include "axiom/memory/linear_allocator.hpp"
#include "axiom/memory/heap_allocator.hpp"

#include <benchmark/benchmark.h>

using namespace axiom::memory;

// ============================================================================
// Benchmark: LinearAllocator vs HeapAllocator - Small allocations
// ============================================================================

static void BM_LinearAllocator_SmallAllocations(benchmark::State& state) {
    const size_t allocSize = 64;
    LinearAllocator allocator(1024 * 1024);  // 1MB

    for (auto _ : state) {
        allocator.reset();
        for (int i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(allocSize, 8);
            benchmark::DoNotOptimize(ptr);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
    state.SetBytesProcessed(state.iterations() * state.range(0) * allocSize);
}

static void BM_HeapAllocator_SmallAllocations(benchmark::State& state) {
    const size_t allocSize = 64;
    HeapAllocator allocator;

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(state.range(0));

        for (int i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(allocSize, 8);
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        // Cleanup
        for (void* ptr : ptrs) {
            allocator.deallocate(ptr, allocSize);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
    state.SetBytesProcessed(state.iterations() * state.range(0) * allocSize);
}

// ============================================================================
// Benchmark: LinearAllocator - Alignment overhead
// ============================================================================

static void BM_LinearAllocator_Alignment8(benchmark::State& state) {
    LinearAllocator allocator(1024 * 1024);

    for (auto _ : state) {
        allocator.reset();
        for (int i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(1, 8);
            benchmark::DoNotOptimize(ptr);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_LinearAllocator_Alignment64(benchmark::State& state) {
    LinearAllocator allocator(1024 * 1024);

    for (auto _ : state) {
        allocator.reset();
        for (int i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(1, 64);
            benchmark::DoNotOptimize(ptr);
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// ============================================================================
// Benchmark: FrameAllocator - Frame-based usage pattern
// ============================================================================

static void BM_FrameAllocator_SimulateFrames(benchmark::State& state) {
    FrameAllocator allocator(2 * 1024 * 1024);  // 2MB total

    for (auto _ : state) {
        // Simulate frame allocations
        for (int i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(128, 16);
            benchmark::DoNotOptimize(ptr);
        }

        // Flip to next frame
        allocator.flip();
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// ============================================================================
// Benchmark: Marker-based reset
// ============================================================================

static void BM_LinearAllocator_MarkerReset(benchmark::State& state) {
    LinearAllocator allocator(1024 * 1024);

    for (auto _ : state) {
        auto marker = allocator.getMarker();

        // Allocate temporary data
        for (int i = 0; i < state.range(0); ++i) {
            void* ptr = allocator.allocate(64, 8);
            benchmark::DoNotOptimize(ptr);
        }

        // Reset to marker
        allocator.resetToMarker(marker);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// ============================================================================
// Benchmark: RAII scope guard
// ============================================================================

static void BM_LinearAllocator_ScopeGuard(benchmark::State& state) {
    LinearAllocator allocator(1024 * 1024);

    for (auto _ : state) {
        {
            LinearAllocatorScope scope(allocator);

            for (int i = 0; i < state.range(0); ++i) {
                void* ptr = allocator.allocate(64, 8);
                benchmark::DoNotOptimize(ptr);
            }
        }  // Automatic reset
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// ============================================================================
// Benchmark: Large allocations
// ============================================================================

static void BM_LinearAllocator_LargeAllocation(benchmark::State& state) {
    const size_t allocSize = state.range(0);
    LinearAllocator allocator(100 * 1024 * 1024);  // 100MB

    for (auto _ : state) {
        allocator.reset();
        void* ptr = allocator.allocate(allocSize, 64);
        benchmark::DoNotOptimize(ptr);
    }

    state.SetBytesProcessed(state.iterations() * allocSize);
}

static void BM_HeapAllocator_LargeAllocation(benchmark::State& state) {
    const size_t allocSize = state.range(0);
    HeapAllocator allocator;

    for (auto _ : state) {
        void* ptr = allocator.allocate(allocSize, 64);
        benchmark::DoNotOptimize(ptr);
        allocator.deallocate(ptr, allocSize);
    }

    state.SetBytesProcessed(state.iterations() * allocSize);
}

// ============================================================================
// Register benchmarks
// ============================================================================

// Small allocations comparison
BENCHMARK(BM_LinearAllocator_SmallAllocations)->Range(100, 10000);
BENCHMARK(BM_HeapAllocator_SmallAllocations)->Range(100, 10000);

// Alignment overhead
BENCHMARK(BM_LinearAllocator_Alignment8)->Range(100, 10000);
BENCHMARK(BM_LinearAllocator_Alignment64)->Range(100, 10000);

// Frame allocator
BENCHMARK(BM_FrameAllocator_SimulateFrames)->Range(10, 1000);

// Marker-based reset
BENCHMARK(BM_LinearAllocator_MarkerReset)->Range(10, 1000);

// RAII scope guard
BENCHMARK(BM_LinearAllocator_ScopeGuard)->Range(10, 1000);

// Large allocations
BENCHMARK(BM_LinearAllocator_LargeAllocation)
    ->Arg(1024 * 1024)      // 1MB
    ->Arg(4 * 1024 * 1024)  // 4MB
    ->Arg(16 * 1024 * 1024) // 16MB
    ->Arg(64 * 1024 * 1024);// 64MB

BENCHMARK(BM_HeapAllocator_LargeAllocation)
    ->Arg(1024 * 1024)      // 1MB
    ->Arg(4 * 1024 * 1024)  // 4MB
    ->Arg(16 * 1024 * 1024) // 16MB
    ->Arg(64 * 1024 * 1024);// 64MB

BENCHMARK_MAIN();
