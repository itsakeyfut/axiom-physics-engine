#pragma once

#include "axiom/memory/allocator.hpp"

#include <cstddef>
#include <cstdint>

namespace axiom::memory {

/**
 * @brief Stack allocator with LIFO allocation/deallocation order
 *
 * StackAllocator provides fast stack-like memory allocation where allocations
 * and deallocations must follow Last-In-First-Out (LIFO) order. Each allocation
 * includes a small header that stores metadata about the previous allocation,
 * allowing the allocator to validate LIFO order in debug builds.
 *
 * This allocator is ideal for:
 * - Physics solver workspace (allocate constraints, solve, deallocate in reverse)
 * - Nested algorithm scratch memory
 * - Temporary data structures with known lifetime order
 * - Function call stacks or recursion workspace
 *
 * Performance characteristics:
 * - Allocation: O(1) - pointer bump + header write
 * - Deallocation: O(1) - LIFO check + pointer restore
 * - Much faster than HeapAllocator for allocation-heavy workloads
 * - Small memory overhead (8-16 bytes per allocation for header)
 *
 * Memory layout:
 * @code
 * [============================== Capacity ==============================]
 * [Header][Data1....][Header][Data2........][Header][Data3...][Free....]
 * ^                                                            ^         ^
 * buffer_                                                      offset_   buffer_ + capacity_
 * @endcode
 *
 * Allocation header structure:
 * @code
 * struct AllocationHeader {
 *     size_t prevOffset;   // Offset of previous allocation (for LIFO checking)
 *     size_t size;         // Size of this allocation (for validation)
 * };
 * @endcode
 *
 * Example usage:
 * @code
 * // Allocate 1MB stack buffer
 * StackAllocator allocator(1024 * 1024);
 *
 * // LIFO allocations and deallocations
 * float* data1 = allocator.allocateArray<float>(100);
 * Vec3* data2 = allocator.allocateArray<Vec3>(50);
 * int* data3 = allocator.allocateArray<int>(200);
 *
 * // Must deallocate in reverse order
 * allocator.deallocateArray(data3, 200);  // OK
 * allocator.deallocateArray(data2, 50);   // OK
 * allocator.deallocateArray(data1, 100);  // OK
 *
 * // LIFO violation (debug build will detect this)
 * // allocator.deallocateArray(data1, 100);  // ERROR: data3 and data2 still allocated!
 *
 * // Using StackArray helper
 * {
 *     StackArray<float> tempData(allocator, 1024);
 *     // ... use tempData.data() ...
 * }  // Automatic LIFO deallocation
 * @endcode
 *
 * @note This allocator is NOT thread-safe
 * @note Deallocations MUST follow LIFO order (enforced in debug builds)
 * @note All memory is reclaimed on destruction
 */
class StackAllocator final : public Allocator {
public:
    /**
     * @brief Construct a StackAllocator with the specified capacity
     *
     * Allocates a contiguous memory block of the given size. The buffer
     * is aligned to the largest common SIMD alignment (64 bytes).
     *
     * @param capacity Size of the memory buffer in bytes
     *
     * @note If allocation fails, the allocator will have zero capacity
     * @note Typical capacity ranges: 256KB-4MB for solver workspace
     */
    explicit StackAllocator(size_t capacity);

    /**
     * @brief Destroy the StackAllocator and free the buffer
     *
     * @warning This does NOT call destructors for allocated objects
     * @warning User must manually destroy all objects before destruction
     * @warning In debug builds, warns if allocations are still active
     */
    ~StackAllocator() override;

    // Disable copy and move (allocators should not be copied/moved)
    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;
    StackAllocator(StackAllocator&&) = delete;
    StackAllocator& operator=(StackAllocator&&) = delete;

    /**
     * @brief Allocate memory with specified size and alignment
     *
     * Allocates memory by:
     * 1. Aligning the current offset to the required alignment
     * 2. Writing an AllocationHeader before the allocation
     * 3. Bumping the offset forward
     *
     * @param size Size in bytes to allocate (must be > 0)
     * @param alignment Alignment requirement in bytes (must be power of 2)
     * @return Pointer to allocated memory, or nullptr if out of memory
     *
     * @note The allocated memory is not initialized
     * @note Returns nullptr if size is 0 or insufficient space remains
     * @note Includes header overhead (typically 16 bytes)
     */
    void* allocate(size_t size, size_t alignment) override;

    /**
     * @brief Deallocate memory (must follow LIFO order)
     *
     * Deallocates memory by:
     * 1. Checking that this is the most recent allocation (LIFO order)
     * 2. Restoring the offset to the previous allocation's offset
     *
     * @param ptr Pointer to memory to deallocate (must be from this allocator)
     * @param size Size in bytes that was originally allocated
     *
     * @note ptr must be the most recently allocated block (LIFO order)
     * @note In debug builds, LIFO violations trigger an assertion
     * @note In release builds, LIFO violations are silently ignored
     * @note Passing nullptr is safe (no-op)
     */
    void deallocate(void* ptr, size_t size) override;

    /**
     * @brief Get total currently allocated size
     *
     * Returns the current offset into the buffer, which represents the
     * total amount of memory that has been allocated (including headers).
     *
     * @return Current offset (number of bytes allocated)
     *
     * @note This includes allocation headers and alignment padding
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
     * @brief Get the total number of deallocations performed
     *
     * Returns the cumulative count of successful deallocate() calls.
     *
     * @return Total deallocation count
     */
    size_t getDeallocationCount() const;

    /**
     * @brief Get the current number of active allocations
     *
     * Returns the number of allocations that have not been deallocated.
     *
     * @return Active allocation count (allocations - deallocations)
     */
    size_t getActiveAllocationCount() const;

    /**
     * @brief Reset the allocator to empty state
     *
     * Resets the offset to zero, effectively reclaiming all allocated
     * memory. This is an O(1) operation.
     *
     * @warning This does NOT call destructors - user must do that manually
     * @warning All pointers allocated from this allocator become invalid
     * @warning In debug builds, warns if allocations are still active
     */
    void reset();

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
     * Resets the peak usage and allocation/deallocation counts to current values.
     * Useful for profiling specific sections of code.
     */
    void resetStatistics();

private:
    /**
     * @brief Allocation header stored before each allocation
     *
     * This header allows the allocator to track the previous allocation
     * offset and validate LIFO order during deallocation.
     */
    struct AllocationHeader {
        size_t prevOffset;  ///< Offset before this allocation
        size_t size;        ///< Size of this allocation (for validation)
    };

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

    /**
     * @brief Get the header for a given allocation pointer
     *
     * @param ptr Pointer to allocated data (not header)
     * @return Pointer to the header immediately before the data
     */
    AllocationHeader* getHeader(void* ptr) const;

    uint8_t* buffer_;           ///< Pre-allocated memory buffer
    size_t capacity_;           ///< Total buffer size in bytes
    size_t offset_;             ///< Current allocation offset
    size_t peakUsage_;          ///< Maximum offset reached
    size_t allocationCount_;    ///< Total number of allocations
    size_t deallocationCount_;  ///< Total number of deallocations
};

/**
 * @brief RAII helper for allocating arrays from StackAllocator
 *
 * StackArray provides automatic LIFO deallocation of arrays allocated
 * from a StackAllocator. The array is allocated on construction and
 * automatically deallocated on destruction.
 *
 * Example usage:
 * @code
 * StackAllocator allocator(1024 * 1024);
 *
 * void processData(StackAllocator& allocator, int count) {
 *     StackArray<float> tempData(allocator, count);
 *
 *     // Use tempData.data() or tempData[i]
 *     for (int i = 0; i < count; ++i) {
 *         tempData[i] = static_cast<float>(i * 2);
 *     }
 *
 *     // ... process data ...
 * }  // Automatic deallocation in LIFO order
 * @endcode
 *
 * @tparam T Element type
 *
 * @note This does NOT call constructors/destructors
 * @note For POD types only (use placement new for complex types)
 * @note Follows LIFO order automatically
 */
template <typename T>
class StackArray {
public:
    /**
     * @brief Construct a StackArray and allocate from the allocator
     *
     * @param allocator StackAllocator to allocate from
     * @param count Number of elements to allocate
     */
    StackArray(StackAllocator& allocator, size_t count)
        : allocator_(allocator), data_(nullptr), count_(count) {
        if (count > 0) {
            data_ = allocator_.allocateArray<T>(count);
        }
    }

    /**
     * @brief Destroy the StackArray and deallocate
     */
    ~StackArray() {
        if (data_ && count_ > 0) {
            allocator_.deallocateArray(data_, count_);
        }
    }

    // Disable copy and move
    StackArray(const StackArray&) = delete;
    StackArray& operator=(const StackArray&) = delete;
    StackArray(StackArray&&) = delete;
    StackArray& operator=(StackArray&&) = delete;

    /**
     * @brief Get pointer to the array data
     *
     * @return Pointer to array, or nullptr if allocation failed
     */
    T* data() { return data_; }

    /**
     * @brief Get const pointer to the array data
     *
     * @return Const pointer to array, or nullptr if allocation failed
     */
    const T* data() const { return data_; }

    /**
     * @brief Get the number of elements in the array
     *
     * @return Element count
     */
    size_t size() const { return count_; }

    /**
     * @brief Check if allocation was successful
     *
     * @return true if data() is not nullptr
     */
    bool isValid() const { return data_ != nullptr; }

    /**
     * @brief Access element by index (no bounds checking)
     *
     * @param index Element index
     * @return Reference to element
     */
    T& operator[](size_t index) { return data_[index]; }

    /**
     * @brief Access element by index (no bounds checking, const)
     *
     * @param index Element index
     * @return Const reference to element
     */
    const T& operator[](size_t index) const { return data_[index]; }

private:
    StackAllocator& allocator_;
    T* data_;
    size_t count_;
};

}  // namespace axiom::memory
