#include "axiom/memory/heap_allocator.hpp"
#include "axiom/memory/pool_allocator.hpp"

#include <benchmark/benchmark.h>

#include <memory>
#include <vector>

using namespace axiom::memory;

// ============================================================================
// Test objects
// ============================================================================

struct SmallObject {
    double data[2];  // 16 bytes
};

struct MediumObject {
    double data[8];  // 64 bytes
};

struct LargeObject {
    double data[32];  // 256 bytes
};

// ============================================================================
// Sequential allocation benchmarks
// ============================================================================

static void BM_PoolAllocator_Sequential_Small(benchmark::State& state) {
    PoolAllocator<sizeof(SmallObject), alignof(SmallObject)> pool(256);

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        // Allocate
        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = pool.allocate(sizeof(SmallObject), alignof(SmallObject));
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        // Deallocate
        for (void* ptr : ptrs) {
            pool.deallocate(ptr, sizeof(SmallObject));
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_PoolAllocator_Sequential_Small)->Range(8, 8192);

static void BM_HeapAllocator_Sequential_Small(benchmark::State& state) {
    HeapAllocator heap;

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        // Allocate
        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = heap.allocate(sizeof(SmallObject), alignof(SmallObject));
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        // Deallocate
        for (void* ptr : ptrs) {
            heap.deallocate(ptr, sizeof(SmallObject));
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_HeapAllocator_Sequential_Small)->Range(8, 8192);

static void BM_StdAllocator_Sequential_Small(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<SmallObject*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        // Allocate
        for (int64_t i = 0; i < state.range(0); ++i) {
            SmallObject* ptr = new SmallObject();
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        // Deallocate
        for (SmallObject* ptr : ptrs) {
            delete ptr;
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StdAllocator_Sequential_Small)->Range(8, 8192);

// ============================================================================
// Medium object benchmarks
// ============================================================================

static void BM_PoolAllocator_Sequential_Medium(benchmark::State& state) {
    PoolAllocator<sizeof(MediumObject), alignof(MediumObject)> pool(256);

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = pool.allocate(sizeof(MediumObject), alignof(MediumObject));
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        for (void* ptr : ptrs) {
            pool.deallocate(ptr, sizeof(MediumObject));
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_PoolAllocator_Sequential_Medium)->Range(8, 8192);

static void BM_HeapAllocator_Sequential_Medium(benchmark::State& state) {
    HeapAllocator heap;

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = heap.allocate(sizeof(MediumObject), alignof(MediumObject));
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        for (void* ptr : ptrs) {
            heap.deallocate(ptr, sizeof(MediumObject));
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_HeapAllocator_Sequential_Medium)->Range(8, 8192);

static void BM_StdAllocator_Sequential_Medium(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<MediumObject*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        for (int64_t i = 0; i < state.range(0); ++i) {
            MediumObject* ptr = new MediumObject();
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        for (MediumObject* ptr : ptrs) {
            delete ptr;
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StdAllocator_Sequential_Medium)->Range(8, 8192);

// ============================================================================
// Allocate/deallocate pattern (simulates object churn)
// ============================================================================

static void BM_PoolAllocator_AllocDealloc_Small(benchmark::State& state) {
    PoolAllocator<sizeof(SmallObject), alignof(SmallObject)> pool(256);

    for (auto _ : state) {
        // Allocate and immediately deallocate
        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = pool.allocate(sizeof(SmallObject), alignof(SmallObject));
            benchmark::DoNotOptimize(ptr);
            pool.deallocate(ptr, sizeof(SmallObject));
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_PoolAllocator_AllocDealloc_Small)->Range(8, 8192);

static void BM_HeapAllocator_AllocDealloc_Small(benchmark::State& state) {
    HeapAllocator heap;

    for (auto _ : state) {
        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = heap.allocate(sizeof(SmallObject), alignof(SmallObject));
            benchmark::DoNotOptimize(ptr);
            heap.deallocate(ptr, sizeof(SmallObject));
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_HeapAllocator_AllocDealloc_Small)->Range(8, 8192);

static void BM_StdAllocator_AllocDealloc_Small(benchmark::State& state) {
    for (auto _ : state) {
        for (int64_t i = 0; i < state.range(0); ++i) {
            SmallObject* ptr = new SmallObject();
            benchmark::DoNotOptimize(ptr);
            delete ptr;
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StdAllocator_AllocDealloc_Small)->Range(8, 8192);

// ============================================================================
// Interleaved allocation/deallocation (simulates realistic usage)
// ============================================================================

static void BM_PoolAllocator_Interleaved(benchmark::State& state) {
    PoolAllocator<sizeof(MediumObject), alignof(MediumObject)> pool(256);

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        // Allocate half
        for (int64_t i = 0; i < state.range(0) / 2; ++i) {
            void* ptr = pool.allocate(sizeof(MediumObject), alignof(MediumObject));
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        // Deallocate every other one
        for (size_t i = 0; i < ptrs.size(); i += 2) {
            pool.deallocate(ptrs[i], sizeof(MediumObject));
            ptrs[i] = nullptr;
        }

        // Allocate more (should reuse deallocated blocks)
        for (int64_t i = 0; i < state.range(0) / 2; ++i) {
            void* ptr = pool.allocate(sizeof(MediumObject), alignof(MediumObject));
            benchmark::DoNotOptimize(ptr);
            const size_t idx = static_cast<size_t>(i * 2);
            if (idx < ptrs.size() && ptrs[idx] == nullptr) {
                ptrs[idx] = ptr;
            } else {
                ptrs.push_back(ptr);
            }
        }

        // Clean up
        for (void* ptr : ptrs) {
            if (ptr) {
                pool.deallocate(ptr, sizeof(MediumObject));
            }
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_PoolAllocator_Interleaved)->Range(64, 4096);

static void BM_HeapAllocator_Interleaved(benchmark::State& state) {
    HeapAllocator heap;

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        for (int64_t i = 0; i < state.range(0) / 2; ++i) {
            void* ptr = heap.allocate(sizeof(MediumObject), alignof(MediumObject));
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        for (size_t i = 0; i < ptrs.size(); i += 2) {
            heap.deallocate(ptrs[i], sizeof(MediumObject));
            ptrs[i] = nullptr;
        }

        for (int64_t i = 0; i < state.range(0) / 2; ++i) {
            void* ptr = heap.allocate(sizeof(MediumObject), alignof(MediumObject));
            benchmark::DoNotOptimize(ptr);
            const size_t idx = static_cast<size_t>(i * 2);
            if (idx < ptrs.size() && ptrs[idx] == nullptr) {
                ptrs[idx] = ptr;
            } else {
                ptrs.push_back(ptr);
            }
        }

        for (void* ptr : ptrs) {
            if (ptr) {
                heap.deallocate(ptr, sizeof(MediumObject));
            }
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_HeapAllocator_Interleaved)->Range(64, 4096);

// ============================================================================
// Reserve performance
// ============================================================================

static void BM_PoolAllocator_Reserve(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        PoolAllocator<sizeof(MediumObject), alignof(MediumObject)> pool(256);
        state.ResumeTiming();

        pool.reserve(static_cast<size_t>(state.range(0)));

        benchmark::DoNotOptimize(pool.capacity());
    }
}
BENCHMARK(BM_PoolAllocator_Reserve)->Range(256, 16384);

// ============================================================================
// Large object benchmarks
// ============================================================================

static void BM_PoolAllocator_Sequential_Large(benchmark::State& state) {
    PoolAllocator<sizeof(LargeObject), alignof(LargeObject)> pool(128);

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = pool.allocate(sizeof(LargeObject), alignof(LargeObject));
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        for (void* ptr : ptrs) {
            pool.deallocate(ptr, sizeof(LargeObject));
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_PoolAllocator_Sequential_Large)->Range(8, 2048);

static void BM_HeapAllocator_Sequential_Large(benchmark::State& state) {
    HeapAllocator heap;

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        for (int64_t i = 0; i < state.range(0); ++i) {
            void* ptr = heap.allocate(sizeof(LargeObject), alignof(LargeObject));
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        for (void* ptr : ptrs) {
            heap.deallocate(ptr, sizeof(LargeObject));
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_HeapAllocator_Sequential_Large)->Range(8, 2048);

static void BM_StdAllocator_Sequential_Large(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<LargeObject*> ptrs;
        ptrs.reserve(static_cast<size_t>(state.range(0)));

        for (int64_t i = 0; i < state.range(0); ++i) {
            LargeObject* ptr = new LargeObject();
            benchmark::DoNotOptimize(ptr);
            ptrs.push_back(ptr);
        }

        for (LargeObject* ptr : ptrs) {
            delete ptr;
        }
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StdAllocator_Sequential_Large)->Range(8, 2048);

// ============================================================================
// Peak throughput test
// ============================================================================

static void BM_PoolAllocator_PeakThroughput(benchmark::State& state) {
    PoolAllocator<sizeof(MediumObject), alignof(MediumObject)> pool(1024);
    pool.reserve(1024);  // Pre-allocate

    for (auto _ : state) {
        void* ptr = pool.allocate(sizeof(MediumObject), alignof(MediumObject));
        benchmark::DoNotOptimize(ptr);
        pool.deallocate(ptr, sizeof(MediumObject));
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PoolAllocator_PeakThroughput);

static void BM_HeapAllocator_PeakThroughput(benchmark::State& state) {
    HeapAllocator heap;

    for (auto _ : state) {
        void* ptr = heap.allocate(sizeof(MediumObject), alignof(MediumObject));
        benchmark::DoNotOptimize(ptr);
        heap.deallocate(ptr, sizeof(MediumObject));
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HeapAllocator_PeakThroughput);

static void BM_StdAllocator_PeakThroughput(benchmark::State& state) {
    for (auto _ : state) {
        MediumObject* ptr = new MediumObject();
        benchmark::DoNotOptimize(ptr);
        delete ptr;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdAllocator_PeakThroughput);

BENCHMARK_MAIN();
