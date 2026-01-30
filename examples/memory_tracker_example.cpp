/**
 * @file memory_tracker_example.cpp
 * @brief Example demonstrating the use of MemoryTracker for leak detection
 *
 * This example shows how to use the MemoryTracker to:
 * - Track allocations from different categories
 * - Detect memory leaks
 * - Generate memory usage reports
 *
 * Note: Memory tracking is only enabled in debug builds (AXIOM_ENABLE_MEMORY_TRACKING)
 */

#include "axiom/memory/memory_tracker.hpp"

#include <iostream>

using namespace axiom::memory;

// Example allocator that uses tracking
class TrackedAllocator {
public:
    void* allocate(size_t size, const char* category) {
        void* ptr = malloc(size);
        AXIOM_TRACK_ALLOC(ptr, size, category);
        return ptr;
    }

    void deallocate(void* ptr) {
        AXIOM_TRACK_DEALLOC(ptr);
        free(ptr);
    }
};

int main() {
    std::cout << "=== Memory Tracker Example ===\n\n";

    TrackedAllocator allocator;

    // Allocate memory for different categories
    std::cout << "1. Allocating memory for different categories...\n";
    void* rigidBody1 = allocator.allocate(1024, "RigidBody");
    void* rigidBody2 = allocator.allocate(2048, "RigidBody");
    void* fluid = allocator.allocate(4096, "Fluid");
    void* softBody = allocator.allocate(512, "SoftBody");

    // Check global statistics
    std::cout << "\n2. Current memory statistics:\n";
    auto& tracker = MemoryTracker::instance();
    auto globalStats = tracker.getStats();
    std::cout << "   Total allocated: " << globalStats.totalAllocated << " bytes\n";
    std::cout << "   Current usage: " << globalStats.currentUsage << " bytes\n";
    std::cout << "   Peak usage: " << globalStats.peakUsage << " bytes\n";

    // Check category-specific statistics
    std::cout << "\n3. Category-specific statistics:\n";
    auto rigidBodyStats = tracker.getStats("RigidBody");
    std::cout << "   [RigidBody] Current usage: " << rigidBodyStats.currentUsage << " bytes ("
              << rigidBodyStats.allocationCount << " allocations)\n";

    auto fluidStats = tracker.getStats("Fluid");
    std::cout << "   [Fluid] Current usage: " << fluidStats.currentUsage << " bytes ("
              << fluidStats.allocationCount << " allocations)\n";

    // Deallocate some memory
    std::cout << "\n4. Deallocating RigidBody memory...\n";
    allocator.deallocate(rigidBody1);
    allocator.deallocate(rigidBody2);

    globalStats = tracker.getStats();
    std::cout << "   Current usage after deallocation: " << globalStats.currentUsage << " bytes\n";

    // Generate comprehensive report
    std::cout << "\n5. Generating comprehensive report:\n";
    tracker.generateReport(std::cout);

    // Intentionally "forget" to deallocate some memory to demonstrate leak detection
    std::cout << "\n6. Detecting memory leaks...\n";
    auto leaks = tracker.detectLeaks();
    if (!leaks.empty()) {
        std::cout << "   Found " << leaks.size() << " memory leak(s)!\n";
        for (const auto& leak : leaks) {
            std::cout << "   - " << leak.size << " bytes allocated at " << leak.file << ":"
                      << leak.line << " [" << leak.category << "]\n";
        }
    } else {
        std::cout << "   No memory leaks detected.\n";
    }

    // Print detailed leak report
    std::cout << "\n7. Detailed leak report:\n";
    tracker.printLeaks();

    // Clean up (to avoid actual leaks in the example)
    allocator.deallocate(fluid);
    allocator.deallocate(softBody);

    std::cout << "\n8. After cleanup:\n";
    leaks = tracker.detectLeaks();
    std::cout << "   Remaining leaks: " << leaks.size() << "\n";

    std::cout << "\n=== Example Complete ===\n";

#ifndef AXIOM_ENABLE_MEMORY_TRACKING
    std::cout << "\nNote: Memory tracking is DISABLED in release builds.\n";
    std::cout << "Build in debug mode to see full tracking output.\n";
#endif

    return 0;
}
