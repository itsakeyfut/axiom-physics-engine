#pragma once

#include "axiom/memory/allocator.hpp"

#include <atomic>
#include <cstddef>

namespace axiom::memory {

/**
 * @brief Heap allocator using system malloc/free
 *
 * HeapAllocator is a concrete allocator implementation that uses the system's
 * malloc/free functions with proper alignment support. It is the default
 * allocator used throughout the Axiom Physics Engine.
 *
 * This allocator provides:
 * - Thread-safe allocation and deallocation
 * - Support for custom alignment requirements (power of 2)
 * - Allocation statistics tracking (total size, count, peak usage)
 * - Cross-platform compatibility (Windows, Linux, macOS)
 *
 * The allocator uses platform-specific aligned allocation functions:
 * - Windows: _aligned_malloc() / _aligned_free()
 * - POSIX: posix_memalign() / free()
 *
 * Example usage:
 * @code
 * // Use the global default allocator
 * Allocator* allocator = getDefaultAllocator();
 * auto* obj = allocator->create<MyClass>();
 * allocator->destroy(obj);
 *
 * // Create a custom heap allocator instance
 * HeapAllocator customAllocator;
 * float* data = customAllocator.allocateArray<float>(1024);
 * customAllocator.deallocateArray(data, 1024);
 *
 * // Check statistics
 * size_t allocated = customAllocator.getAllocatedSize();
 * size_t peak = customAllocator.getPeakAllocatedSize();
 * @endcode
 *
 * @note This allocator is thread-safe for concurrent allocations/deallocations
 * @note The global default allocator is an instance of HeapAllocator
 * @note All statistics tracking uses lock-free atomic operations
 */
class HeapAllocator final : public Allocator {
public:
    /**
     * @brief Construct a new HeapAllocator
     *
     * Initializes all statistics counters to zero.
     */
    HeapAllocator() = default;

    /**
     * @brief Destroy the HeapAllocator
     *
     * @warning Does not free any allocated memory - user must manually
     *          deallocate all memory before destroying the allocator
     */
    ~HeapAllocator() override = default;

    // Disable copy and move (allocators should not be copied/moved)
    HeapAllocator(const HeapAllocator&) = delete;
    HeapAllocator& operator=(const HeapAllocator&) = delete;
    HeapAllocator(HeapAllocator&&) = delete;
    HeapAllocator& operator=(HeapAllocator&&) = delete;

    /**
     * @brief Allocate memory with specified size and alignment
     *
     * Allocates memory using the system's aligned allocation functions.
     * The returned pointer is guaranteed to be aligned to at least
     * 'alignment' bytes.
     *
     * @param size Size in bytes to allocate (must be > 0)
     * @param alignment Alignment requirement in bytes (must be power of 2)
     * @return Pointer to allocated memory, or nullptr on failure
     *
     * @note The allocated memory is not initialized
     * @note Thread-safe: can be called concurrently from multiple threads
     * @note Updates allocation statistics atomically
     */
    void* allocate(size_t size, size_t alignment) override;

    /**
     * @brief Deallocate previously allocated memory
     *
     * Frees memory that was previously allocated by this allocator.
     *
     * @param ptr Pointer to memory to deallocate (must be from this allocator)
     * @param size Size in bytes that was originally allocated
     *
     * @note ptr must have been allocated by this allocator instance
     * @note size must match the size passed to allocate()
     * @note Passing nullptr is safe (no-op)
     * @note Thread-safe: can be called concurrently from multiple threads
     * @note Updates deallocation statistics atomically
     */
    void deallocate(void* ptr, size_t size) override;

    /**
     * @brief Get total currently allocated size
     *
     * Returns the total number of bytes currently allocated by this
     * allocator (allocated size - deallocated size).
     *
     * @return Total number of bytes currently allocated
     *
     * @note This includes alignment padding added by the system
     * @note Thread-safe: can be called concurrently with allocate/deallocate
     * @note Uses atomic load with relaxed memory ordering
     */
    size_t getAllocatedSize() const override;

    /**
     * @brief Get the total number of allocations performed
     *
     * Returns the cumulative count of all allocate() calls made on this
     * allocator instance since construction.
     *
     * @return Total allocation count
     *
     * @note Thread-safe: can be called concurrently
     * @note This count never decreases (cumulative total)
     */
    size_t getAllocationCount() const;

    /**
     * @brief Get the total number of deallocations performed
     *
     * Returns the cumulative count of all deallocate() calls made on this
     * allocator instance since construction.
     *
     * @return Total deallocation count
     *
     * @note Thread-safe: can be called concurrently
     * @note This count never decreases (cumulative total)
     * @note If getAllocationCount() == getDeallocationCount(), no leaks exist
     */
    size_t getDeallocationCount() const;

    /**
     * @brief Get the peak allocated size
     *
     * Returns the maximum value that getAllocatedSize() has reached
     * during the lifetime of this allocator.
     *
     * @return Maximum allocated size reached (in bytes)
     *
     * @note Thread-safe: uses lock-free compare-and-swap for updates
     * @note Useful for tracking memory usage high-water marks
     * @note This value never decreases
     */
    size_t getPeakAllocatedSize() const;

private:
    /**
     * @brief Update peak allocated size if current exceeds it
     *
     * Uses compare-and-swap in a loop to atomically update the peak
     * if the current allocated size is greater.
     */
    void updatePeak();

    std::atomic<size_t> allocatedSize_{0};      ///< Current allocated size
    std::atomic<size_t> allocationCount_{0};    ///< Total allocations
    std::atomic<size_t> deallocationCount_{0};  ///< Total deallocations
    std::atomic<size_t> peakAllocatedSize_{0};  ///< Peak memory usage
};

}  // namespace axiom::memory
