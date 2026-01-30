#pragma once

#include "axiom/memory/allocator.hpp"

#include <limits>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace axiom::memory {

/**
 * @brief STL-compatible allocator adapter for axiom::memory::Allocator
 *
 * StlAllocatorAdapter adapts Axiom's custom Allocator interface to work with
 * standard library containers (std::vector, std::map, etc.). This allows using
 * custom memory management strategies (Pool, Linear, Stack allocators) with STL
 * containers, enabling unified memory management throughout the engine.
 *
 * The adapter is fully compliant with std::allocator_traits requirements and
 * supports all standard container operations including copy, move, swap, and
 * rebind (necessary for node-based containers like std::map).
 *
 * Key features:
 * - Compatible with all STL containers
 * - Supports custom allocators (Heap, Pool, Linear, Stack)
 * - Proper propagation semantics for move/copy operations
 * - Memory statistics tracking via underlying allocator
 * - Zero overhead over direct allocator usage
 *
 * Template parameters:
 * @tparam T Element type to allocate
 *
 * Example usage:
 * @code
 * // Using default heap allocator
 * std::vector<int, StlAllocatorAdapter<int>> vec1;
 * vec1.push_back(42);
 *
 * // Using custom pool allocator
 * PoolAllocator<64> pool;
 * std::vector<int, StlAllocatorAdapter<int>> vec2(
 *     StlAllocatorAdapter<int>(&pool));
 * vec2.push_back(42);
 *
 * // Using linear allocator for temporary data
 * LinearAllocator linearAlloc(1024 * 1024);  // 1MB
 * std::vector<float, StlAllocatorAdapter<float>> tempData(
 *     StlAllocatorAdapter<float>(&linearAlloc));
 * // ... use tempData ...
 * linearAlloc.reset();  // Free all at once
 *
 * // Type aliases for convenience
 * Vector<int> vec3;  // Same as std::vector<int, StlAllocatorAdapter<int>>
 * @endcode
 *
 * @note The allocator pointer must remain valid for the lifetime of all
 *       containers using this adapter
 * @note This adapter does NOT take ownership of the allocator pointer
 * @note For thread-safety, use a thread-safe underlying allocator
 */
template <typename T>
class StlAllocatorAdapter {
public:
    // ========================================================================
    // Type definitions required by std::allocator_traits
    // ========================================================================

    /// Type of elements allocated
    using value_type = T;

    /// Type for representing sizes
    using size_type = std::size_t;

    /// Type for representing differences between pointers
    using difference_type = std::ptrdiff_t;

    /// Propagate allocator on container move assignment
    using propagate_on_container_move_assignment = std::true_type;

    /// Allocators with different backing allocators are not equal
    using is_always_equal = std::false_type;

    // ========================================================================
    // Constructors and assignment
    // ========================================================================

    /**
     * @brief Construct adapter with specific allocator
     *
     * @param allocator Backing allocator (defaults to getDefaultAllocator())
     *
     * @note The allocator pointer is not owned by this adapter
     * @note The allocator must remain valid for the lifetime of all containers
     *       using this adapter
     */
    explicit StlAllocatorAdapter(Allocator* allocator = getDefaultAllocator()) noexcept
        : allocator_(allocator) {}

    /**
     * @brief Copy constructor
     *
     * @param other Adapter to copy from
     */
    StlAllocatorAdapter(const StlAllocatorAdapter& other) noexcept = default;

    /**
     * @brief Rebind constructor for different type
     *
     * This constructor is used by std::allocator_traits to create allocators
     * for different types (e.g., when std::map needs to allocate tree nodes
     * internally).
     *
     * @tparam U Type of the other adapter
     * @param other Adapter to rebind from
     */
    template <typename U>
    StlAllocatorAdapter(const StlAllocatorAdapter<U>& other) noexcept
        : allocator_(other.allocator_) {}

    /**
     * @brief Copy assignment operator
     */
    StlAllocatorAdapter& operator=(const StlAllocatorAdapter&) noexcept = default;

    // ========================================================================
    // Allocation and deallocation
    // ========================================================================

    /**
     * @brief Allocate memory for n objects of type T
     *
     * Allocates uninitialized memory for n objects of type T with proper
     * alignment. Throws std::bad_alloc on failure (as required by the standard).
     *
     * @param n Number of objects to allocate space for
     * @return Pointer to allocated memory
     *
     * @throws std::bad_array_new_length if n is too large
     * @throws std::bad_alloc if allocation fails
     *
     * @note The returned memory is not initialized
     * @note The returned pointer has alignment of alignof(T)
     * @note Time complexity depends on the underlying allocator
     */
    [[nodiscard]] T* allocate(size_type n) {
        // Check for overflow
        if (n > std::numeric_limits<size_type>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }

        // Calculate total size
        const size_type totalSize = n * sizeof(T);

        // Allocate from underlying allocator
        void* ptr = allocator_->allocate(totalSize, alignof(T));

        // Throw on failure (as required by std::allocator)
        if (!ptr) {
            throw std::bad_alloc();
        }

        return static_cast<T*>(ptr);
    }

    /**
     * @brief Deallocate memory for n objects
     *
     * Deallocates memory previously allocated with allocate(). The pointer
     * must have been returned by a prior call to allocate() with the same n.
     *
     * @param ptr Pointer to memory to deallocate (must be from allocate())
     * @param n Number of objects (must match the n passed to allocate())
     *
     * @note This does NOT call destructors - the container handles that
     * @note Passing nullptr is safe (no-op)
     * @note n must match the value passed to allocate()
     */
    void deallocate(T* ptr, size_type n) noexcept {
        if (!ptr) {
            return;
        }

        const size_type totalSize = n * sizeof(T);
        allocator_->deallocate(ptr, totalSize);
    }

    // ========================================================================
    // Accessors
    // ========================================================================

    /**
     * @brief Get the underlying allocator
     *
     * Returns a pointer to the Allocator that this adapter delegates to.
     * Useful for accessing allocator statistics or custom methods.
     *
     * @return Pointer to underlying allocator (never nullptr)
     *
     * Example:
     * @code
     * Vector<int> vec;
     * size_t allocated = vec.get_allocator().getAllocator()->getAllocatedSize();
     * @endcode
     */
    Allocator* getAllocator() const noexcept { return allocator_; }

    // ========================================================================
    // Comparison operators
    // ========================================================================

    /**
     * @brief Equality comparison
     *
     * Two adapters are equal if they use the same underlying allocator.
     * This is required for container swap and splice operations.
     *
     * @tparam U Type of the other adapter
     * @param other Adapter to compare with
     * @return true if both adapters use the same allocator, false otherwise
     */
    template <typename U>
    bool operator==(const StlAllocatorAdapter<U>& other) const noexcept {
        return allocator_ == other.allocator_;
    }

    /**
     * @brief Inequality comparison
     *
     * @tparam U Type of the other adapter
     * @param other Adapter to compare with
     * @return true if adapters use different allocators, false otherwise
     */
    template <typename U>
    bool operator!=(const StlAllocatorAdapter<U>& other) const noexcept {
        return !(*this == other);
    }

private:
    // Allow other specializations to access allocator_
    template <typename U>
    friend class StlAllocatorAdapter;

    Allocator* allocator_;  ///< Pointer to backing allocator (not owned)
};

// ============================================================================
// Convenient type aliases for common containers
// ============================================================================

/**
 * @brief Vector using StlAllocatorAdapter
 *
 * Alias for std::vector with default StlAllocatorAdapter. This provides a
 * convenient way to use std::vector with Axiom's memory management.
 *
 * Example:
 * @code
 * // Using default heap allocator
 * Vector<int> vec1;
 *
 * // Using custom allocator
 * LinearAllocator linearAlloc(1024 * 1024);
 * Vector<float> vec2(StlAllocatorAdapter<float>(&linearAlloc));
 * @endcode
 *
 * @tparam T Element type
 */
template <typename T>
using Vector = std::vector<T, StlAllocatorAdapter<T>>;

/**
 * @brief Map using StlAllocatorAdapter
 *
 * Alias for std::map with default StlAllocatorAdapter.
 *
 * @tparam K Key type
 * @tparam V Value type
 * @tparam Compare Comparison function (default: std::less<K>)
 */
template <typename K, typename V, typename Compare = std::less<K>>
using Map = std::map<K, V, Compare, StlAllocatorAdapter<std::pair<const K, V>>>;

/**
 * @brief Set using StlAllocatorAdapter
 *
 * Alias for std::set with default StlAllocatorAdapter.
 *
 * @tparam T Element type
 * @tparam Compare Comparison function (default: std::less<T>)
 */
template <typename T, typename Compare = std::less<T>>
using Set = std::set<T, Compare, StlAllocatorAdapter<T>>;

/**
 * @brief Unordered map using StlAllocatorAdapter
 *
 * Alias for std::unordered_map with default StlAllocatorAdapter.
 *
 * @tparam K Key type
 * @tparam V Value type
 * @tparam Hash Hash function (default: std::hash<K>)
 * @tparam KeyEqual Equality function (default: std::equal_to<K>)
 */
template <typename K, typename V, typename Hash = std::hash<K>,
          typename KeyEqual = std::equal_to<K>>
using UnorderedMap =
    std::unordered_map<K, V, Hash, KeyEqual, StlAllocatorAdapter<std::pair<const K, V>>>;

/**
 * @brief Unordered set using StlAllocatorAdapter
 *
 * Alias for std::unordered_set with default StlAllocatorAdapter.
 *
 * @tparam T Element type
 * @tparam Hash Hash function (default: std::hash<T>)
 * @tparam KeyEqual Equality function (default: std::equal_to<T>)
 */
template <typename T, typename Hash = std::hash<T>, typename KeyEqual = std::equal_to<T>>
using UnorderedSet = std::unordered_set<T, Hash, KeyEqual, StlAllocatorAdapter<T>>;

}  // namespace axiom::memory
