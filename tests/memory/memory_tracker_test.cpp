#include "axiom/memory/memory_tracker.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <thread>
#include <vector>

using namespace axiom::memory;

// ============================================================================
// Test fixture
// ============================================================================

class MemoryTrackerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset tracker state before each test
        MemoryTracker::instance().reset();
    }

    void TearDown() override {
        // Clean up after each test
        MemoryTracker::instance().reset();
    }
};

// ============================================================================
// Basic tracking tests
// ============================================================================

TEST_F(MemoryTrackerTest, SingleAllocation) {
    auto& tracker = MemoryTracker::instance();

    // Allocate memory
    void* ptr = malloc(100);
    ASSERT_NE(ptr, nullptr);

    tracker.recordAllocation(ptr, 100, "Test", __FILE__, __LINE__);

    // Check statistics
    auto stats = tracker.getStats();
    EXPECT_EQ(stats.totalAllocated, 100);
    EXPECT_EQ(stats.currentUsage, 100);
    EXPECT_EQ(stats.peakUsage, 100);
    EXPECT_EQ(stats.allocationCount, 1);
    EXPECT_EQ(stats.deallocationCount, 0);
    EXPECT_EQ(stats.getActiveAllocationCount(), 1);

    // Check leak detection
    auto leaks = tracker.detectLeaks();
    EXPECT_EQ(leaks.size(), 1);
    EXPECT_EQ(leaks[0].ptr, ptr);
    EXPECT_EQ(leaks[0].size, 100);

    // Deallocate
    tracker.recordDeallocation(ptr);
    free(ptr);

    // Verify statistics after deallocation
    stats = tracker.getStats();
    EXPECT_EQ(stats.totalAllocated, 100);
    EXPECT_EQ(stats.totalDeallocated, 100);
    EXPECT_EQ(stats.currentUsage, 0);
    EXPECT_EQ(stats.peakUsage, 100);
    EXPECT_EQ(stats.allocationCount, 1);
    EXPECT_EQ(stats.deallocationCount, 1);
    EXPECT_EQ(stats.getActiveAllocationCount(), 0);

    // No leaks
    leaks = tracker.detectLeaks();
    EXPECT_TRUE(leaks.empty());
}

TEST_F(MemoryTrackerTest, MultipleAllocations) {
    auto& tracker = MemoryTracker::instance();

    std::vector<void*> ptrs;
    const size_t count = 10;
    const size_t size = 64;

    // Allocate multiple blocks
    for (size_t i = 0; i < count; ++i) {
        void* ptr = malloc(size);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
        tracker.recordAllocation(ptr, size, "Test", __FILE__, __LINE__);
    }

    // Check statistics
    auto stats = tracker.getStats();
    EXPECT_EQ(stats.totalAllocated, count * size);
    EXPECT_EQ(stats.currentUsage, count * size);
    EXPECT_EQ(stats.peakUsage, count * size);
    EXPECT_EQ(stats.allocationCount, count);
    EXPECT_EQ(stats.deallocationCount, 0);

    // Deallocate half
    for (size_t i = 0; i < count / 2; ++i) {
        tracker.recordDeallocation(ptrs[i]);
        free(ptrs[i]);
    }

    // Check after partial deallocation
    stats = tracker.getStats();
    EXPECT_EQ(stats.currentUsage, (count - count / 2) * size);
    EXPECT_EQ(stats.peakUsage, count * size);  // Peak should remain
    EXPECT_EQ(stats.deallocationCount, count / 2);

    // Deallocate remaining
    for (size_t i = count / 2; i < count; ++i) {
        tracker.recordDeallocation(ptrs[i]);
        free(ptrs[i]);
    }

    // All deallocated
    stats = tracker.getStats();
    EXPECT_EQ(stats.currentUsage, 0);
    EXPECT_EQ(stats.totalAllocated, stats.totalDeallocated);
    EXPECT_TRUE(tracker.detectLeaks().empty());
}

TEST_F(MemoryTrackerTest, PeakUsageTracking) {
    auto& tracker = MemoryTracker::instance();

    // Allocate increasing amounts
    void* ptr1 = malloc(100);
    tracker.recordAllocation(ptr1, 100, "Test", __FILE__, __LINE__);
    EXPECT_EQ(tracker.getStats().peakUsage, 100);

    void* ptr2 = malloc(200);
    tracker.recordAllocation(ptr2, 200, "Test", __FILE__, __LINE__);
    EXPECT_EQ(tracker.getStats().peakUsage, 300);

    void* ptr3 = malloc(300);
    tracker.recordAllocation(ptr3, 300, "Test", __FILE__, __LINE__);
    EXPECT_EQ(tracker.getStats().peakUsage, 600);

    // Deallocate middle allocation
    tracker.recordDeallocation(ptr2);
    free(ptr2);
    EXPECT_EQ(tracker.getStats().currentUsage, 400);
    EXPECT_EQ(tracker.getStats().peakUsage, 600);  // Peak unchanged

    // Allocate smaller amount
    void* ptr4 = malloc(50);
    tracker.recordAllocation(ptr4, 50, "Test", __FILE__, __LINE__);
    EXPECT_EQ(tracker.getStats().currentUsage, 450);
    EXPECT_EQ(tracker.getStats().peakUsage, 600);  // Peak still unchanged

    // Clean up
    tracker.recordDeallocation(ptr1);
    free(ptr1);
    tracker.recordDeallocation(ptr3);
    free(ptr3);
    tracker.recordDeallocation(ptr4);
    free(ptr4);
}

// ============================================================================
// Category-based tracking tests
// ============================================================================

TEST_F(MemoryTrackerTest, CategoryTracking) {
    auto& tracker = MemoryTracker::instance();

    // Allocate for different categories
    void* ptr1 = malloc(100);
    tracker.recordAllocation(ptr1, 100, "RigidBody", __FILE__, __LINE__);

    void* ptr2 = malloc(200);
    tracker.recordAllocation(ptr2, 200, "Fluid", __FILE__, __LINE__);

    void* ptr3 = malloc(150);
    tracker.recordAllocation(ptr3, 150, "RigidBody", __FILE__, __LINE__);

    // Check global statistics
    auto globalStats = tracker.getStats();
    EXPECT_EQ(globalStats.totalAllocated, 450);
    EXPECT_EQ(globalStats.currentUsage, 450);
    EXPECT_EQ(globalStats.allocationCount, 3);

    // Check RigidBody category
    auto rigidBodyStats = tracker.getStats("RigidBody");
    EXPECT_EQ(rigidBodyStats.totalAllocated, 250);
    EXPECT_EQ(rigidBodyStats.currentUsage, 250);
    EXPECT_EQ(rigidBodyStats.allocationCount, 2);

    // Check Fluid category
    auto fluidStats = tracker.getStats("Fluid");
    EXPECT_EQ(fluidStats.totalAllocated, 200);
    EXPECT_EQ(fluidStats.currentUsage, 200);
    EXPECT_EQ(fluidStats.allocationCount, 1);

    // Deallocate RigidBody allocation
    tracker.recordDeallocation(ptr1);
    free(ptr1);

    rigidBodyStats = tracker.getStats("RigidBody");
    EXPECT_EQ(rigidBodyStats.currentUsage, 150);
    EXPECT_EQ(rigidBodyStats.totalDeallocated, 100);

    // Fluid should be unchanged
    fluidStats = tracker.getStats("Fluid");
    EXPECT_EQ(fluidStats.currentUsage, 200);

    // Clean up
    tracker.recordDeallocation(ptr2);
    free(ptr2);
    tracker.recordDeallocation(ptr3);
    free(ptr3);
}

TEST_F(MemoryTrackerTest, UnknownCategory) {
    auto& tracker = MemoryTracker::instance();

    // Query non-existent category
    auto stats = tracker.getStats("NonExistent");
    EXPECT_EQ(stats.totalAllocated, 0);
    EXPECT_EQ(stats.currentUsage, 0);
    EXPECT_EQ(stats.allocationCount, 0);
}

TEST_F(MemoryTrackerTest, NullCategory) {
    auto& tracker = MemoryTracker::instance();

    // Allocation with null category should still work
    void* ptr = malloc(100);
    tracker.recordAllocation(ptr, 100, nullptr, __FILE__, __LINE__);

    auto stats = tracker.getStats();
    EXPECT_EQ(stats.totalAllocated, 100);

    // Clean up
    tracker.recordDeallocation(ptr);
    free(ptr);
}

// ============================================================================
// Leak detection tests
// ============================================================================

TEST_F(MemoryTrackerTest, LeakDetection) {
    auto& tracker = MemoryTracker::instance();

    // Create some leaks
    void* ptr1 = malloc(100);
    tracker.recordAllocation(ptr1, 100, "Test", "file1.cpp", 10);

    void* ptr2 = malloc(200);
    tracker.recordAllocation(ptr2, 200, "Test", "file2.cpp", 20);

    void* ptr3 = malloc(50);
    tracker.recordAllocation(ptr3, 50, "Test", "file3.cpp", 30);

    // Detect leaks
    auto leaks = tracker.detectLeaks();
    ASSERT_EQ(leaks.size(), 3);

    // Leaks should be sorted by size (largest first)
    EXPECT_EQ(leaks[0].size, 200);
    EXPECT_EQ(leaks[0].ptr, ptr2);
    EXPECT_STREQ(leaks[0].file, "file2.cpp");
    EXPECT_EQ(leaks[0].line, 20);

    EXPECT_EQ(leaks[1].size, 100);
    EXPECT_EQ(leaks[1].ptr, ptr1);

    EXPECT_EQ(leaks[2].size, 50);
    EXPECT_EQ(leaks[2].ptr, ptr3);

    // Clean up (avoid actual leaks in test)
    free(ptr1);
    free(ptr2);
    free(ptr3);
}

TEST_F(MemoryTrackerTest, NoLeaks) {
    auto& tracker = MemoryTracker::instance();

    void* ptr = malloc(100);
    tracker.recordAllocation(ptr, 100, "Test", __FILE__, __LINE__);
    tracker.recordDeallocation(ptr);
    free(ptr);

    auto leaks = tracker.detectLeaks();
    EXPECT_TRUE(leaks.empty());
}

// ============================================================================
// Edge cases
// ============================================================================

TEST_F(MemoryTrackerTest, NullPointerAllocation) {
    auto& tracker = MemoryTracker::instance();

    // Recording null pointer should be safe (no-op)
    tracker.recordAllocation(nullptr, 100, "Test", __FILE__, __LINE__);

    auto stats = tracker.getStats();
    EXPECT_EQ(stats.allocationCount, 0);
}

TEST_F(MemoryTrackerTest, NullPointerDeallocation) {
    auto& tracker = MemoryTracker::instance();

    // Deallocating null pointer should be safe (no-op)
    tracker.recordDeallocation(nullptr);

    auto stats = tracker.getStats();
    EXPECT_EQ(stats.deallocationCount, 0);
}

TEST_F(MemoryTrackerTest, DoubleFree) {
    auto& tracker = MemoryTracker::instance();

    void* ptr = malloc(100);
    tracker.recordAllocation(ptr, 100, "Test", __FILE__, __LINE__);
    tracker.recordDeallocation(ptr);

    // Second deallocation should print warning but not crash
    // Redirect stderr to check warning (optional)
    tracker.recordDeallocation(ptr);

    auto stats = tracker.getStats();
    EXPECT_EQ(stats.deallocationCount, 1);  // Only one counted

    free(ptr);
}

TEST_F(MemoryTrackerTest, UnknownPointerDeallocation) {
    auto& tracker = MemoryTracker::instance();

    void* ptr = malloc(100);

    // Deallocate without recording allocation
    // Should print warning but not crash
    tracker.recordDeallocation(ptr);

    auto stats = tracker.getStats();
    EXPECT_EQ(stats.deallocationCount, 0);

    free(ptr);
}

// ============================================================================
// Report generation tests
// ============================================================================

TEST_F(MemoryTrackerTest, GenerateReport) {
    auto& tracker = MemoryTracker::instance();

    // Create some allocations
    void* ptr1 = malloc(100);
    tracker.recordAllocation(ptr1, 100, "RigidBody", __FILE__, __LINE__);

    void* ptr2 = malloc(200);
    tracker.recordAllocation(ptr2, 200, "Fluid", __FILE__, __LINE__);

    // Generate report
    std::ostringstream oss;
    tracker.generateReport(oss);

    std::string report = oss.str();

    // Check that report contains expected information
    EXPECT_NE(report.find("MEMORY TRACKER REPORT"), std::string::npos);
    EXPECT_NE(report.find("Global Statistics"), std::string::npos);
    EXPECT_NE(report.find("Category Statistics"), std::string::npos);
    EXPECT_NE(report.find("RigidBody"), std::string::npos);
    EXPECT_NE(report.find("Fluid"), std::string::npos);
    EXPECT_NE(report.find("300"), std::string::npos);  // Total allocated

    // Clean up
    tracker.recordDeallocation(ptr1);
    free(ptr1);
    tracker.recordDeallocation(ptr2);
    free(ptr2);
}

TEST_F(MemoryTrackerTest, PrintLeaks) {
    auto& tracker = MemoryTracker::instance();

    // Create leak
    void* ptr = malloc(100);
    tracker.recordAllocation(ptr, 100, "Test", __FILE__, __LINE__);

    // Print leaks (output goes to stderr)
    // This test just verifies it doesn't crash
    tracker.printLeaks();

    // Clean up
    free(ptr);
}

// ============================================================================
// Thread safety tests
// ============================================================================

TEST_F(MemoryTrackerTest, ConcurrentAllocations) {
    auto& tracker = MemoryTracker::instance();

    const size_t threadCount = 4;
    const size_t allocationsPerThread = 100;

    std::vector<std::thread> threads;
    std::vector<std::vector<void*>> threadPtrs(threadCount);

    // Launch threads
    for (size_t t = 0; t < threadCount; ++t) {
        threads.emplace_back([&tracker, &threadPtrs, t]() {
            for (size_t i = 0; i < allocationsPerThread; ++i) {
                void* ptr = malloc(64);
                if (ptr) {
                    tracker.recordAllocation(ptr, 64, "Test", __FILE__, __LINE__);
                    threadPtrs[t].push_back(ptr);
                }
            }
        });
    }

    // Wait for threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify statistics
    auto stats = tracker.getStats();
    EXPECT_EQ(stats.allocationCount, threadCount * allocationsPerThread);
    EXPECT_EQ(stats.currentUsage, threadCount * allocationsPerThread * 64);

    // Deallocate all
    for (size_t t = 0; t < threadCount; ++t) {
        for (void* ptr : threadPtrs[t]) {
            tracker.recordDeallocation(ptr);
            free(ptr);
        }
    }

    // Verify cleanup
    stats = tracker.getStats();
    EXPECT_EQ(stats.currentUsage, 0);
    EXPECT_TRUE(tracker.detectLeaks().empty());
}

// ============================================================================
// Reset tests
// ============================================================================

TEST_F(MemoryTrackerTest, Reset) {
    auto& tracker = MemoryTracker::instance();

    void* ptr = malloc(100);
    tracker.recordAllocation(ptr, 100, "Test", __FILE__, __LINE__);

    // Reset tracker
    tracker.reset();

    // Statistics should be zeroed
    auto stats = tracker.getStats();
    EXPECT_EQ(stats.totalAllocated, 0);
    EXPECT_EQ(stats.currentUsage, 0);
    EXPECT_EQ(stats.allocationCount, 0);

    // No leaks detected (memory was cleared)
    auto leaks = tracker.detectLeaks();
    EXPECT_TRUE(leaks.empty());

    // Clean up actual memory
    free(ptr);
}

// ============================================================================
// Macro tests (conditional compilation)
// ============================================================================

#ifdef AXIOM_ENABLE_MEMORY_TRACKING
TEST_F(MemoryTrackerTest, MacrosEnabled) {
    auto& tracker = MemoryTracker::instance();

    void* ptr = malloc(100);
    AXIOM_TRACK_ALLOC(ptr, 100, "Test");

    auto stats = tracker.getStats();
    EXPECT_EQ(stats.allocationCount, 1);

    AXIOM_TRACK_DEALLOC(ptr);
    free(ptr);

    stats = tracker.getStats();
    EXPECT_EQ(stats.deallocationCount, 1);
}
#else
TEST_F(MemoryTrackerTest, MacrosDisabled) {
    // When macros are disabled, they should compile to no-ops
    void* ptr = malloc(100);
    AXIOM_TRACK_ALLOC(ptr, 100, "Test");
    AXIOM_TRACK_DEALLOC(ptr);
    free(ptr);

    // Test passes if it compiles and runs without error
    SUCCEED();
}
#endif
