#include "axiom/memory/heap_allocator.hpp"

#include "axiom/memory/allocator.hpp"

#include <atomic>
#include <cstdlib>
#include <mutex>

#ifdef _WIN32
#include <malloc.h>  // _aligned_malloc, _aligned_free
#else
#include <cstdlib>  // posix_memalign, free
#endif

namespace axiom::memory {

// ============================================================================
// Cross-platform aligned allocation
// ============================================================================

void* alignedAlloc(size_t size, size_t alignment) {
    if (size == 0) {
        return nullptr;
    }

    // Alignment must be power of 2
    if ((alignment & (alignment - 1)) != 0) {
        return nullptr;
    }

#ifdef _WIN32
    // Windows: use _aligned_malloc
    return _aligned_malloc(size, alignment);
#else
    // POSIX: use posix_memalign (more portable than aligned_alloc)
    // posix_memalign requires alignment to be a multiple of sizeof(void*)
    size_t actualAlignment = alignment;
    if (actualAlignment < sizeof(void*)) {
        actualAlignment = sizeof(void*);
    }

    void* ptr = nullptr;
    if (posix_memalign(&ptr, actualAlignment, size) != 0) {
        return nullptr;
    }
    return ptr;
#endif
}

void alignedFree(void* ptr) {
    if (!ptr) {
        return;
    }

#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

// ============================================================================
// HeapAllocator implementation
// ============================================================================

void* HeapAllocator::allocate(size_t size, size_t alignment) {
    if (size == 0) {
        return nullptr;
    }

    // Allocate aligned memory
    void* ptr = alignedAlloc(size, alignment);
    if (!ptr) {
        return nullptr;
    }

    // Track statistics
    allocatedSize_.fetch_add(size, std::memory_order_relaxed);
    allocationCount_.fetch_add(1, std::memory_order_relaxed);

    // Update peak
    updatePeak();

    return ptr;
}

void HeapAllocator::deallocate(void* ptr, size_t size) {
    if (!ptr) {
        return;
    }

    alignedFree(ptr);

    // Update statistics
    allocatedSize_.fetch_sub(size, std::memory_order_relaxed);
    deallocationCount_.fetch_add(1, std::memory_order_relaxed);
}

size_t HeapAllocator::getAllocatedSize() const {
    return allocatedSize_.load(std::memory_order_relaxed);
}

size_t HeapAllocator::getAllocationCount() const {
    return allocationCount_.load(std::memory_order_relaxed);
}

size_t HeapAllocator::getDeallocationCount() const {
    return deallocationCount_.load(std::memory_order_relaxed);
}

size_t HeapAllocator::getPeakAllocatedSize() const {
    return peakAllocatedSize_.load(std::memory_order_relaxed);
}

void HeapAllocator::updatePeak() {
    size_t current = allocatedSize_.load(std::memory_order_relaxed);
    size_t peak = peakAllocatedSize_.load(std::memory_order_relaxed);

    while (current > peak) {
        if (peakAllocatedSize_.compare_exchange_weak(peak, current,
                                                     std::memory_order_relaxed)) {
            break;
        }
    }
}

// ============================================================================
// Global default allocator
// ============================================================================

namespace {
// Global default allocator instance
HeapAllocator g_defaultAllocator;

// Current allocator pointer (can be overridden)
std::atomic<Allocator*> g_currentAllocator{&g_defaultAllocator};

// Mutex for setDefaultAllocator (not needed for getDefaultAllocator)
std::mutex g_allocatorMutex;
}  // namespace

Allocator* getDefaultAllocator() {
    return g_currentAllocator.load(std::memory_order_acquire);
}

Allocator* setDefaultAllocator(Allocator* allocator) {
    if (!allocator) {
        // Cannot set nullptr as default allocator
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(g_allocatorMutex);
    Allocator* previous = g_currentAllocator.exchange(allocator, std::memory_order_release);
    return previous;
}

}  // namespace axiom::memory
