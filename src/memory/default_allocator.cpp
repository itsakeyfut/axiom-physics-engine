#include "axiom/memory/allocator.hpp"

#include <atomic>
#include <cstdlib>
#include <mutex>

#ifdef _WIN32
#include <malloc.h>  // _aligned_malloc, _aligned_free
#else
#include <cstdlib>  // aligned_alloc, free (C11)
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
    // POSIX: use aligned_alloc (C11)
    // Note: aligned_alloc requires size to be a multiple of alignment
    size_t adjustedSize = ((size + alignment - 1) / alignment) * alignment;
    return aligned_alloc(alignment, adjustedSize);
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
// DefaultAllocator implementation
// ============================================================================

/**
 * @brief Default allocator implementation using system malloc/free
 *
 * This allocator uses the system's malloc/free with proper alignment support.
 * It tracks allocation statistics (total allocated size, allocation count)
 * and is thread-safe.
 */
class DefaultAllocator final : public Allocator {
public:
    DefaultAllocator() = default;
    ~DefaultAllocator() override = default;

    // Disable copy and move
    DefaultAllocator(const DefaultAllocator&) = delete;
    DefaultAllocator& operator=(const DefaultAllocator&) = delete;
    DefaultAllocator(DefaultAllocator&&) = delete;
    DefaultAllocator& operator=(DefaultAllocator&&) = delete;

    void* allocate(size_t size, size_t alignment) override {
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

    void deallocate(void* ptr, size_t size) override {
        if (!ptr) {
            return;
        }

        alignedFree(ptr);

        // Update statistics
        allocatedSize_.fetch_sub(size, std::memory_order_relaxed);
        deallocationCount_.fetch_add(1, std::memory_order_relaxed);
    }

    size_t getAllocatedSize() const override {
        return allocatedSize_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get the total number of allocations performed
     * @return Total allocation count
     */
    size_t getAllocationCount() const { return allocationCount_.load(std::memory_order_relaxed); }

    /**
     * @brief Get the total number of deallocations performed
     * @return Total deallocation count
     */
    size_t getDeallocationCount() const {
        return deallocationCount_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get the peak allocated size
     * @return Maximum allocated size reached
     */
    size_t getPeakAllocatedSize() const {
        return peakAllocatedSize_.load(std::memory_order_relaxed);
    }

private:
    void updatePeak() {
        size_t current = allocatedSize_.load(std::memory_order_relaxed);
        size_t peak = peakAllocatedSize_.load(std::memory_order_relaxed);

        while (current > peak) {
            if (peakAllocatedSize_.compare_exchange_weak(peak, current,
                                                         std::memory_order_relaxed)) {
                break;
            }
        }
    }

    std::atomic<size_t> allocatedSize_{0};
    std::atomic<size_t> allocationCount_{0};
    std::atomic<size_t> deallocationCount_{0};
    std::atomic<size_t> peakAllocatedSize_{0};
};

// ============================================================================
// Global default allocator
// ============================================================================

namespace {
// Global default allocator instance
DefaultAllocator g_defaultAllocator;

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
