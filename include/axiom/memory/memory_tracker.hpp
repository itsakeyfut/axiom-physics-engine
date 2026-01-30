#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace axiom::memory {

/**
 * @brief Memory allocation tracking system for leak detection and usage analysis
 *
 * MemoryTracker records all memory allocations and deallocations, providing
 * detailed statistics about memory usage across different categories (modules).
 * This is primarily used in debug builds to detect memory leaks and analyze
 * memory usage patterns.
 *
 * Features:
 * - Tracks allocation/deallocation with source location (file, line)
 * - Detects memory leaks at program exit
 * - Provides per-category (module) statistics
 * - Thread-safe implementation
 * - Zero overhead in release builds (compile-time disabled)
 *
 * Example usage:
 * @code
 * // In allocator implementation
 * void* ptr = malloc(size);
 * AXIOM_TRACK_ALLOC(ptr, size, "RigidBody");
 *
 * free(ptr);
 * AXIOM_TRACK_DEALLOC(ptr);
 *
 * // At program exit
 * auto leaks = MemoryTracker::instance().detectLeaks();
 * if (!leaks.empty()) {
 *     MemoryTracker::instance().printLeaks();
 * }
 * @endcode
 */
class MemoryTracker {
public:
    /**
     * @brief Get singleton instance of MemoryTracker
     *
     * @return Reference to the global MemoryTracker instance
     */
    static MemoryTracker& instance();

    /**
     * @brief Statistics for memory allocations
     */
    struct Stats {
        size_t totalAllocated = 0;     ///< Total bytes allocated (cumulative)
        size_t totalDeallocated = 0;   ///< Total bytes deallocated (cumulative)
        size_t currentUsage = 0;       ///< Current memory usage in bytes
        size_t peakUsage = 0;          ///< Peak memory usage in bytes
        size_t allocationCount = 0;    ///< Number of allocations
        size_t deallocationCount = 0;  ///< Number of deallocations

        /**
         * @brief Get number of active (not freed) allocations
         */
        size_t getActiveAllocationCount() const { return allocationCount - deallocationCount; }
    };

    /**
     * @brief Information about a memory leak
     */
    struct LeakInfo {
        void* ptr;             ///< Pointer to leaked memory
        size_t size;           ///< Size of allocation in bytes
        const char* category;  ///< Category/module name
        const char* file;      ///< Source file where allocation occurred
        int line;              ///< Line number where allocation occurred
    };

    /**
     * @brief Record a memory allocation
     *
     * @param ptr Pointer to allocated memory (must not be nullptr)
     * @param size Size of allocation in bytes
     * @param category Category/module name (e.g., "RigidBody", "Fluid")
     * @param file Source file where allocation occurred
     * @param line Line number where allocation occurred
     *
     * @note This function is thread-safe
     * @note If ptr is nullptr, the call is ignored
     */
    void recordAllocation(void* ptr, size_t size, const char* category, const char* file, int line);

    /**
     * @brief Record a memory deallocation
     *
     * @param ptr Pointer to memory being deallocated
     *
     * @note This function is thread-safe
     * @note If ptr is nullptr or not tracked, the call is ignored
     * @note If ptr was not previously allocated, a warning is logged
     */
    void recordDeallocation(void* ptr);

    /**
     * @brief Get statistics for all allocations or a specific category
     *
     * @param category Category name, or nullptr for global statistics
     * @return Statistics structure
     *
     * @note This function is thread-safe
     * @note If category is not found, returns zeroed statistics
     */
    Stats getStats(const char* category = nullptr) const;

    /**
     * @brief Detect memory leaks
     *
     * Returns information about all allocations that have not been deallocated.
     *
     * @return Vector of leak information structures
     *
     * @note This function is thread-safe
     * @note Call this at program exit to detect leaks
     */
    std::vector<LeakInfo> detectLeaks() const;

    /**
     * @brief Print memory leak report to standard error
     *
     * Prints detailed information about all detected memory leaks,
     * including allocation location and size.
     *
     * @note This function is thread-safe
     */
    void printLeaks() const;

    /**
     * @brief Generate comprehensive memory usage report
     *
     * Writes a detailed report of memory usage statistics to the
     * provided output stream, including global and per-category statistics.
     *
     * @param out Output stream to write report to
     *
     * @note This function is thread-safe
     */
    void generateReport(std::ostream& out) const;

    /**
     * @brief Reset all tracking data
     *
     * Clears all allocation records and statistics. Use this carefully,
     * as it may hide memory leaks.
     *
     * @note This function is thread-safe
     */
    void reset();

    // Singleton - no copy or move
    MemoryTracker(const MemoryTracker&) = delete;
    MemoryTracker& operator=(const MemoryTracker&) = delete;
    MemoryTracker(MemoryTracker&&) = delete;
    MemoryTracker& operator=(MemoryTracker&&) = delete;

private:
    MemoryTracker() = default;
    ~MemoryTracker() = default;

    /**
     * @brief Record of a single allocation
     */
    struct AllocationRecord {
        void* ptr;
        size_t size;
        const char* category;
        const char* file;
        int line;
    };

    // Active allocations (ptr -> record)
    mutable std::mutex mutex_;
    std::unordered_map<void*, AllocationRecord> allocations_;

    // Per-category statistics (category -> stats)
    std::unordered_map<std::string, Stats> categoryStats_;

    // Global statistics
    Stats globalStats_;
};

}  // namespace axiom::memory

// ============================================================================
// Tracking macros
// ============================================================================

#ifdef AXIOM_ENABLE_MEMORY_TRACKING
/**
 * @brief Track memory allocation (enabled in debug builds)
 *
 * @param ptr Pointer to allocated memory
 * @param size Size of allocation in bytes
 * @param category Category/module name
 */
#define AXIOM_TRACK_ALLOC(ptr, size, category)                                                     \
    ::axiom::memory::MemoryTracker::instance().recordAllocation(ptr, size, category, __FILE__,     \
                                                                __LINE__)

/**
 * @brief Track memory deallocation (enabled in debug builds)
 *
 * @param ptr Pointer to memory being deallocated
 */
#define AXIOM_TRACK_DEALLOC(ptr) ::axiom::memory::MemoryTracker::instance().recordDeallocation(ptr)
#else
   // No-op in release builds (zero overhead)
#define AXIOM_TRACK_ALLOC(ptr, size, category) ((void)0)
#define AXIOM_TRACK_DEALLOC(ptr) ((void)0)
#endif
