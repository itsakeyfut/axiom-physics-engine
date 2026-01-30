#include "axiom/memory/memory_tracker.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace axiom::memory {

// ============================================================================
// Singleton instance
// ============================================================================

MemoryTracker& MemoryTracker::instance() {
    static MemoryTracker instance;
    return instance;
}

// ============================================================================
// Allocation tracking
// ============================================================================

void MemoryTracker::recordAllocation(void* ptr, size_t size, const char* category, const char* file,
                                     int line) {
    if (!ptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Record allocation
    AllocationRecord record{ptr, size, category, file, line};
    allocations_[ptr] = record;

    // Update global statistics
    globalStats_.totalAllocated += size;
    globalStats_.currentUsage += size;
    globalStats_.allocationCount++;

    // Update peak usage
    if (globalStats_.currentUsage > globalStats_.peakUsage) {
        globalStats_.peakUsage = globalStats_.currentUsage;
    }

    // Update category statistics
    if (category) {
        Stats& categoryStats = categoryStats_[category];
        categoryStats.totalAllocated += size;
        categoryStats.currentUsage += size;
        categoryStats.allocationCount++;

        // Update category peak
        if (categoryStats.currentUsage > categoryStats.peakUsage) {
            categoryStats.peakUsage = categoryStats.currentUsage;
        }
    }
}

void MemoryTracker::recordDeallocation(void* ptr) {
    if (!ptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Find allocation record
    auto it = allocations_.find(ptr);
    if (it == allocations_.end()) {
        // Pointer not tracked - could be a double-free or external allocation
        std::cerr << "[MemoryTracker] Warning: Attempted to deallocate "
                  << "untracked pointer: " << ptr << std::endl;
        return;
    }

    const AllocationRecord& record = it->second;

    // Update global statistics
    globalStats_.totalDeallocated += record.size;
    globalStats_.currentUsage -= record.size;
    globalStats_.deallocationCount++;

    // Update category statistics
    if (record.category) {
        Stats& categoryStats = categoryStats_[record.category];
        categoryStats.totalDeallocated += record.size;
        categoryStats.currentUsage -= record.size;
        categoryStats.deallocationCount++;
    }

    // Remove allocation record
    allocations_.erase(it);
}

// ============================================================================
// Statistics
// ============================================================================

MemoryTracker::Stats MemoryTracker::getStats(const char* category) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!category) {
        // Return global statistics
        return globalStats_;
    }

    // Return category statistics (zeroed if not found)
    auto it = categoryStats_.find(category);
    if (it != categoryStats_.end()) {
        return it->second;
    }

    return Stats{};
}

// ============================================================================
// Leak detection
// ============================================================================

std::vector<MemoryTracker::LeakInfo> MemoryTracker::detectLeaks() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<LeakInfo> leaks;
    leaks.reserve(allocations_.size());

    for (const auto& [ptr, record] : allocations_) {
        LeakInfo leak{ptr, record.size, record.category, record.file, record.line};
        leaks.push_back(leak);
    }

    // Sort by size (largest first) for easier analysis
    std::sort(leaks.begin(), leaks.end(),
              [](const LeakInfo& a, const LeakInfo& b) { return a.size > b.size; });

    return leaks;
}

void MemoryTracker::printLeaks() const {
    auto leaks = detectLeaks();

    if (leaks.empty()) {
        std::cerr << "[MemoryTracker] No memory leaks detected.\n";
        return;
    }

    std::cerr << "\n";
    std::cerr << "========================================\n";
    std::cerr << "MEMORY LEAK REPORT\n";
    std::cerr << "========================================\n";
    std::cerr << "Total leaks: " << leaks.size() << "\n";

    // Calculate total leaked memory
    size_t totalLeaked = 0;
    for (const auto& leak : leaks) {
        totalLeaked += leak.size;
    }
    std::cerr << "Total leaked: " << totalLeaked << " bytes\n";
    std::cerr << "========================================\n\n";

    // Print individual leaks
    for (size_t i = 0; i < leaks.size(); ++i) {
        const auto& leak = leaks[i];
        std::cerr << "Leak #" << (i + 1) << ":\n";
        std::cerr << "  Address:  " << leak.ptr << "\n";
        std::cerr << "  Size:     " << leak.size << " bytes\n";
        std::cerr << "  Category: " << (leak.category ? leak.category : "Unknown") << "\n";
        std::cerr << "  Location: " << (leak.file ? leak.file : "Unknown") << ":" << leak.line
                  << "\n";
        std::cerr << "\n";
    }

    std::cerr << "========================================\n";
}

// ============================================================================
// Reporting
// ============================================================================

void MemoryTracker::generateReport(std::ostream& out) const {
    std::lock_guard<std::mutex> lock(mutex_);

    out << "\n";
    out << "========================================\n";
    out << "MEMORY TRACKER REPORT\n";
    out << "========================================\n\n";

    // Global statistics
    out << "Global Statistics:\n";
    out << "  Total allocated:     " << globalStats_.totalAllocated << " bytes\n";
    out << "  Total deallocated:   " << globalStats_.totalDeallocated << " bytes\n";
    out << "  Current usage:       " << globalStats_.currentUsage << " bytes\n";
    out << "  Peak usage:          " << globalStats_.peakUsage << " bytes\n";
    out << "  Allocation count:    " << globalStats_.allocationCount << "\n";
    out << "  Deallocation count:  " << globalStats_.deallocationCount << "\n";
    out << "  Active allocations:  " << globalStats_.getActiveAllocationCount() << "\n";
    out << "\n";

    // Category statistics
    if (!categoryStats_.empty()) {
        out << "Category Statistics:\n";
        out << "----------------------------------------\n";

        // Sort categories by current usage (descending)
        std::vector<std::pair<std::string, Stats>> sortedCategories(categoryStats_.begin(),
                                                                    categoryStats_.end());
        std::sort(sortedCategories.begin(), sortedCategories.end(),
                  [](const auto& a, const auto& b) {
                      return a.second.currentUsage > b.second.currentUsage;
                  });

        for (const auto& [category, stats] : sortedCategories) {
            out << "\n[" << category << "]\n";
            out << "  Total allocated:     " << stats.totalAllocated << " bytes\n";
            out << "  Total deallocated:   " << stats.totalDeallocated << " bytes\n";
            out << "  Current usage:       " << stats.currentUsage << " bytes\n";
            out << "  Peak usage:          " << stats.peakUsage << " bytes\n";
            out << "  Allocation count:    " << stats.allocationCount << "\n";
            out << "  Deallocation count:  " << stats.deallocationCount << "\n";
            out << "  Active allocations:  " << stats.getActiveAllocationCount() << "\n";
        }

        out << "\n";
    }

    out << "========================================\n";
}

void MemoryTracker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    allocations_.clear();
    categoryStats_.clear();
    globalStats_ = Stats{};
}

}  // namespace axiom::memory
