#pragma once

#include "axiom/memory/allocator.hpp"

#include <cstddef>
#include <cstdint>

namespace axiom::memory {

/**
 * @brief Linear allocator for fast sequential memory allocation
 *
 * LinearAllocator provides extremely fast O(1) allocation by simply bumping
 * a pointer forward through a pre-allocated memory block. Individual
 * deallocations are not supported - instead, memory is reclaimed in bulk
 * via reset() or resetToMarker().
 *
 * This allocator is ideal for:
 * - Frame-based temporary allocations
 * - Scratch memory for algorithms
 * - Building temporary data structures
 * - Physics solver workspace
 *
 * Performance characteristics:
 * - Allocation: O(1) - just a pointer bump and alignment adjustment
 * - Deallocation: No-op (individual deallocate() does nothing)
 * - Reset: O(1) - resets offset to zero
 * - 100-1000x faster than HeapAllocator for allocation-heavy workloads
 *
 * Memory layout:
 * @code
 * [============================== Capacity ==============================]
 * [Used Memory............][Free Memory.................................]
 * ^                        ^                                            ^
 * buffer_                  offset_                                      buffer_ + capacity_
 * @endcode
 *
 * Example usage:
 * @code
 * // Allocate 1MB linear buffer
 * LinearAllocator allocator(1024 * 1024);
 *
 * // Fast allocations
 * float* data1 = allocator.allocateArray<float>(1024);
 * Vec3* vertices = allocator.allocateArray<Vec3>(512);
 *
 * // Use the data...
 *
 * // Reset and reuse
 * allocator.reset();
 *
 * // Using markers for partial reset
 * auto marker = allocator.getMarker();
 * float* temp = allocator.allocateArray<float>(100);
 * // ... use temp ...
 * allocator.resetToMarker(marker);  // Only reclaim 'temp'
 *
 * // RAII scope guard
 * {
 *     LinearAllocatorScope scope(allocator);
 *     float* scopedData = allocator.allocateArray<float>(200);
 *     // ... use scopedData ...
 * }  // Automatic reset to marker on scope exit
 * @endcode
 *
 * @note This allocator is NOT thread-safe
 * @note Individual deallocate() is a no-op
 * @note All memory is reclaimed on destruction
 */
class LinearAllocator final : public Allocator {
public:
    /**
     * @brief Marker type for saving/restoring allocator state
     *
     * A marker represents the current offset into the buffer. It can be
     * saved and later restored to reclaim all allocations made after the
     * marker was created.
     */
    using Marker = size_t;

    /**
     * @brief Construct a LinearAllocator with the specified capacity
     *
     * Allocates a contiguous memory block of the given size. The buffer
     * is aligned to the largest common SIMD alignment (64 bytes).
     *
     * @param capacity Size of the memory buffer in bytes
     *
     * @note If allocation fails, the allocator will have zero capacity
     * @note Typical capacity ranges: 1MB-16MB for frame allocators
     */
    explicit LinearAllocator(size_t capacity);

    /**
     * @brief Destroy the LinearAllocator and free the buffer
     *
     * @warning This does NOT call destructors for allocated objects
     * @warning User must manually destroy all objects before destruction
     */
    ~LinearAllocator() override;

    // Disable copy and move (allocators should not be copied/moved)
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;
    LinearAllocator(LinearAllocator&&) = delete;
    LinearAllocator& operator=(LinearAllocator&&) = delete;

    /**
     * @brief Allocate memory with specified size and alignment
     *
     * Allocates memory by bumping the current offset forward, with proper
     * alignment adjustment. This is an O(1) operation.
     *
     * @param size Size in bytes to allocate (must be > 0)
     * @param alignment Alignment requirement in bytes (must be power of 2)
     * @return Pointer to allocated memory, or nullptr if out of memory
     *
     * @note The allocated memory is not initialized
     * @note Returns nullptr if size is 0 or insufficient space remains
     * @note Alignment padding is automatically added as needed
     */
    void* allocate(size_t size, size_t alignment) override;

    /**
     * @brief Deallocate memory (no-op for linear allocator)
     *
     * LinearAllocator does not support individual deallocation. This
     * function does nothing. Use reset() or resetToMarker() to reclaim
     * memory in bulk.
     *
     * @param ptr Pointer to memory (ignored)
     * @param size Size in bytes (ignored)
     *
     * @note This is a no-op - linear allocators only support bulk reset
     */
    void deallocate(void* ptr, size_t size) override;

    /**
     * @brief Get total currently allocated size
     *
     * Returns the current offset into the buffer, which represents the
     * total amount of memory that has been allocated.
     *
     * @return Current offset (number of bytes allocated)
     *
     * @note This includes alignment padding
     * @note This does NOT include unused capacity
     */
    size_t getAllocatedSize() const override;

    /**
     * @brief Get the total capacity of the allocator
     *
     * Returns the total size of the pre-allocated buffer.
     *
     * @return Total capacity in bytes
     */
    size_t capacity() const;

    /**
     * @brief Get the remaining free space
     *
     * Returns the number of bytes still available for allocation.
     *
     * @return Remaining capacity (capacity - offset)
     */
    size_t remaining() const;

    /**
     * @brief Get peak memory usage
     *
     * Returns the maximum offset reached during the lifetime of this
     * allocator (or since the last resetStatistics() call).
     *
     * @return Peak offset in bytes
     */
    size_t getPeakUsage() const;

    /**
     * @brief Get the total number of allocations performed
     *
     * Returns the cumulative count of successful allocate() calls.
     *
     * @return Total allocation count
     */
    size_t getAllocationCount() const;

    /**
     * @brief Reset the allocator to empty state
     *
     * Resets the offset to zero, effectively reclaiming all allocated
     * memory. This is an O(1) operation.
     *
     * @warning This does NOT call destructors - user must do that manually
     * @warning All pointers allocated from this allocator become invalid
     */
    void reset();

    /**
     * @brief Save the current allocator state as a marker
     *
     * Returns a marker representing the current offset. This marker can
     * be passed to resetToMarker() to reclaim all allocations made after
     * this point.
     *
     * @return Marker representing current offset
     *
     * Example:
     * @code
     * auto marker = allocator.getMarker();
     * float* temp = allocator.allocateArray<float>(100);
     * // ... use temp ...
     * allocator.resetToMarker(marker);  // Reclaim temp
     * @endcode
     */
    Marker getMarker() const;

    /**
     * @brief Reset the allocator to a previously saved marker
     *
     * Resets the offset to the value stored in the marker, effectively
     * reclaiming all allocations made after the marker was created.
     *
     * @param marker Marker to reset to (from getMarker())
     *
     * @warning This does NOT call destructors - user must do that manually
     * @warning All pointers allocated after the marker become invalid
     * @warning If marker > current offset, this is a no-op
     */
    void resetToMarker(Marker marker);

    /**
     * @brief Check if a pointer was allocated from this allocator
     *
     * Checks if the given pointer is within this allocator's buffer range.
     *
     * @param ptr Pointer to check
     * @return true if ptr is within the buffer, false otherwise
     *
     * @note This only checks if ptr is in range, not if it was actually allocated
     * @note Useful for debugging and validation
     */
    bool owns(void* ptr) const;

    /**
     * @brief Reset allocation statistics
     *
     * Resets the peak usage and allocation count to current values.
     * Useful for profiling specific sections of code.
     */
    void resetStatistics();

private:
    /**
     * @brief Align a value up to the specified alignment
     *
     * @param value Value to align
     * @param alignment Alignment (must be power of 2)
     * @return Aligned value
     */
    static size_t alignUp(size_t value, size_t alignment);

    /**
     * @brief Update peak usage if current offset exceeds it
     */
    void updatePeak();

    uint8_t* buffer_;         ///< Pre-allocated memory buffer
    size_t capacity_;         ///< Total buffer size in bytes
    size_t offset_;           ///< Current allocation offset
    size_t peakUsage_;        ///< Maximum offset reached
    size_t allocationCount_;  ///< Total number of allocations
};

/**
 * @brief RAII scope guard for LinearAllocator
 *
 * LinearAllocatorScope saves the allocator's current marker on construction
 * and automatically resets to that marker on destruction. This ensures that
 * all allocations made within the scope are reclaimed when the scope exits.
 *
 * Example usage:
 * @code
 * LinearAllocator allocator(1024 * 1024);
 *
 * {
 *     LinearAllocatorScope scope(allocator);
 *     float* temp1 = allocator.allocateArray<float>(100);
 *     Vec3* temp2 = allocator.allocateArray<Vec3>(50);
 *     // ... use temp data ...
 * }  // Automatic reset - temp1 and temp2 are reclaimed
 *
 * // Allocator is back to state before scope
 * @endcode
 *
 * @note This does NOT call destructors - user must do that manually
 */
class LinearAllocatorScope {
public:
    /**
     * @brief Construct a scope guard and save the current marker
     *
     * @param allocator LinearAllocator to guard
     */
    explicit LinearAllocatorScope(LinearAllocator& allocator);

    /**
     * @brief Destroy the scope guard and reset to the saved marker
     */
    ~LinearAllocatorScope();

    // Disable copy and move
    LinearAllocatorScope(const LinearAllocatorScope&) = delete;
    LinearAllocatorScope& operator=(const LinearAllocatorScope&) = delete;
    LinearAllocatorScope(LinearAllocatorScope&&) = delete;
    LinearAllocatorScope& operator=(LinearAllocatorScope&&) = delete;

private:
    LinearAllocator& allocator_;
    LinearAllocator::Marker marker_;
};

/**
 * @brief Double-buffered frame allocator
 *
 * FrameAllocator manages two LinearAllocator instances and alternates between
 * them each frame. This allows one buffer to be used for allocations while
 * the other buffer's memory from the previous frame is automatically reset.
 *
 * This pattern is common in game engines and real-time simulations:
 * - Frame N: Allocate from buffer A, buffer B is unused
 * - flip(): Switch buffers, reset A
 * - Frame N+1: Allocate from buffer B, buffer A is unused
 * - flip(): Switch buffers, reset B
 * - ...
 *
 * Example usage:
 * @code
 * // 2MB per buffer (4MB total)
 * FrameAllocator frameAlloc(2 * 1024 * 1024);
 *
 * while (simulationRunning) {
 *     // Allocate temporary data for this frame
 *     float* contacts = frameAlloc.allocateArray<float>(numContacts * 7);
 *     Vec3* normals = frameAlloc.allocateArray<Vec3>(numContacts);
 *
 *     // ... simulate physics ...
 *
 *     // Flip to next frame (automatically resets previous buffer)
 *     frameAlloc.flip();
 * }
 * @endcode
 *
 * @note This allocator is NOT thread-safe (flip() must be single-threaded)
 * @note Each buffer has half the total memory capacity
 */
class FrameAllocator final : public Allocator {
public:
    /**
     * @brief Construct a FrameAllocator with the specified total capacity
     *
     * Allocates two LinearAllocator buffers, each with capacity/2 bytes.
     *
     * @param totalCapacity Total memory budget (split between two buffers)
     *
     * @note Each buffer has capacity = totalCapacity / 2
     * @note Starts with buffer 0 as the current buffer
     */
    explicit FrameAllocator(size_t totalCapacity);

    /**
     * @brief Destroy the FrameAllocator and free both buffers
     */
    ~FrameAllocator() override;

    // Disable copy and move
    FrameAllocator(const FrameAllocator&) = delete;
    FrameAllocator& operator=(const FrameAllocator&) = delete;
    FrameAllocator(FrameAllocator&&) = delete;
    FrameAllocator& operator=(FrameAllocator&&) = delete;

    /**
     * @brief Allocate memory from the current frame buffer
     *
     * @param size Size in bytes to allocate
     * @param alignment Alignment requirement (must be power of 2)
     * @return Pointer to allocated memory, or nullptr if out of memory
     */
    void* allocate(size_t size, size_t alignment) override;

    /**
     * @brief Deallocate memory (no-op)
     *
     * @param ptr Pointer to memory (ignored)
     * @param size Size in bytes (ignored)
     */
    void deallocate(void* ptr, size_t size) override;

    /**
     * @brief Get total currently allocated size across both buffers
     *
     * @return Sum of allocated sizes in both buffers
     */
    size_t getAllocatedSize() const override;

    /**
     * @brief Switch to the next buffer and reset the previous one
     *
     * This is typically called once per frame. It:
     * 1. Switches currentBuffer_ to the other buffer
     * 2. Resets the (previously current, now previous) buffer
     *
     * @warning This does NOT call destructors for objects in the previous buffer
     * @warning All pointers from the previous frame become invalid
     * @warning This is NOT thread-safe - must be called from a single thread
     */
    void flip();

    /**
     * @brief Get the current frame number
     *
     * Returns the number of times flip() has been called.
     *
     * @return Frame counter
     */
    size_t getFrameNumber() const;

    /**
     * @brief Get the current buffer's capacity
     *
     * Returns the capacity of a single buffer (totalCapacity / 2).
     *
     * @return Per-buffer capacity in bytes
     */
    size_t getBufferCapacity() const;

    /**
     * @brief Get the current buffer's remaining space
     *
     * @return Remaining capacity in the current buffer
     */
    size_t remaining() const;

    /**
     * @brief Get peak usage across both buffers
     *
     * Returns the maximum of the two buffers' peak usage.
     *
     * @return Peak usage in bytes
     */
    size_t getPeakUsage() const;

private:
    LinearAllocator* buffers_[2];  ///< Two alternating buffers
    size_t currentBuffer_;         ///< Index of current buffer (0 or 1)
    size_t frameNumber_;           ///< Number of flips (frame counter)
};

}  // namespace axiom::memory
