#include "axiom/memory/linear_allocator.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

using namespace axiom::memory;

// ============================================================================
// Test utilities
// ============================================================================

struct TestObject {
    int value;
    double padding;

    TestObject() : value(0), padding(0.0) { ++constructCount; }
    explicit TestObject(int v) : value(v), padding(0.0) { ++constructCount; }
    ~TestObject() { ++destructCount; }

    static void resetCounters() {
        constructCount = 0;
        destructCount = 0;
    }

    static int constructCount;
    static int destructCount;
};

int TestObject::constructCount = 0;
int TestObject::destructCount = 0;

// ============================================================================
// LinearAllocator tests
// ============================================================================

TEST(LinearAllocatorTest, DefaultConstruction) {
    LinearAllocator allocator(1024);
    EXPECT_EQ(allocator.capacity(), 1024);
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
    EXPECT_EQ(allocator.remaining(), 1024);
    EXPECT_EQ(allocator.getPeakUsage(), 0);
    EXPECT_EQ(allocator.getAllocationCount(), 0);
}

TEST(LinearAllocatorTest, ZeroCapacity) {
    LinearAllocator allocator(0);
    EXPECT_EQ(allocator.capacity(), 0);
    EXPECT_EQ(allocator.remaining(), 0);

    void* ptr = allocator.allocate(100, 8);
    EXPECT_EQ(ptr, nullptr);
}

TEST(LinearAllocatorTest, SingleAllocation) {
    LinearAllocator allocator(1024);

    void* ptr = allocator.allocate(100, 8);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), 100);
    EXPECT_EQ(allocator.remaining(), 924);
    EXPECT_EQ(allocator.getAllocationCount(), 1);
    EXPECT_TRUE(allocator.owns(ptr));
}

TEST(LinearAllocatorTest, MultipleAllocations) {
    LinearAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    void* ptr2 = allocator.allocate(200, 8);
    void* ptr3 = allocator.allocate(300, 8);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);

    // Size may include alignment padding
    EXPECT_GE(allocator.getAllocatedSize(), 600);
    EXPECT_LE(allocator.getAllocatedSize(), 650);
    EXPECT_EQ(allocator.getAllocationCount(), 3);

    // Pointers should be sequential
    EXPECT_TRUE(allocator.owns(ptr1));
    EXPECT_TRUE(allocator.owns(ptr2));
    EXPECT_TRUE(allocator.owns(ptr3));
}

TEST(LinearAllocatorTest, AlignmentVerification) {
    LinearAllocator allocator(1024);

    // Test various alignments
    void* ptr8 = allocator.allocate(10, 8);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr8) % 8, 0);

    void* ptr16 = allocator.allocate(10, 16);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr16) % 16, 0);

    void* ptr32 = allocator.allocate(10, 32);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr32) % 32, 0);

    void* ptr64 = allocator.allocate(10, 64);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr64) % 64, 0);
}

TEST(LinearAllocatorTest, AlignmentPadding) {
    LinearAllocator allocator(1024);

    // Allocate 1 byte with 1-byte alignment
    void* ptr1 = allocator.allocate(1, 1);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), 1);

    // Allocate 1 byte with 16-byte alignment - should add padding
    void* ptr2 = allocator.allocate(1, 16);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_GE(allocator.getAllocatedSize(), 17);  // At least 1 + 16
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 16, 0);
}

TEST(LinearAllocatorTest, OutOfMemory) {
    LinearAllocator allocator(100);

    void* ptr1 = allocator.allocate(50, 8);
    ASSERT_NE(ptr1, nullptr);

    void* ptr2 = allocator.allocate(40, 8);
    ASSERT_NE(ptr2, nullptr);

    // This should fail - not enough space
    void* ptr3 = allocator.allocate(20, 8);
    EXPECT_EQ(ptr3, nullptr);
}

TEST(LinearAllocatorTest, Reset) {
    LinearAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    void* ptr2 = allocator.allocate(200, 8);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    size_t sizeBeforeReset = allocator.getAllocatedSize();
    EXPECT_GE(sizeBeforeReset, 300);

    allocator.reset();
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
    EXPECT_EQ(allocator.remaining(), 1024);

    // Should be able to allocate again
    void* ptr3 = allocator.allocate(500, 8);
    ASSERT_NE(ptr3, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), 500);
}

TEST(LinearAllocatorTest, MarkerSaveRestore) {
    LinearAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    ASSERT_NE(ptr1, nullptr);

    // Save marker
    auto marker = allocator.getMarker();
    EXPECT_EQ(marker, 100);

    // Allocate more
    void* ptr2 = allocator.allocate(200, 8);
    void* ptr3 = allocator.allocate(300, 8);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);
    size_t sizeAfterAllocations = allocator.getAllocatedSize();
    EXPECT_GE(sizeAfterAllocations, 600);

    // Reset to marker
    allocator.resetToMarker(marker);
    EXPECT_EQ(allocator.getAllocatedSize(), marker);
    EXPECT_EQ(allocator.remaining(), 1024 - marker);

    // Should be able to allocate from marker point
    void* ptr4 = allocator.allocate(400, 8);
    ASSERT_NE(ptr4, nullptr);
    EXPECT_GE(allocator.getAllocatedSize(), marker + 400);
}

TEST(LinearAllocatorTest, MarkerInvalidReset) {
    LinearAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    ASSERT_NE(ptr1, nullptr);

    auto marker = allocator.getMarker();
    EXPECT_EQ(marker, 100);

    // Try to reset to a marker beyond current offset (should be no-op)
    allocator.resetToMarker(marker + 1000);
    EXPECT_EQ(allocator.getAllocatedSize(), 100);
}

TEST(LinearAllocatorTest, PeakUsageTracking) {
    LinearAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    ASSERT_NE(ptr1, nullptr);
    size_t peak1 = allocator.getPeakUsage();
    EXPECT_EQ(peak1, 100);

    void* ptr2 = allocator.allocate(200, 8);
    ASSERT_NE(ptr2, nullptr);
    size_t peak2 = allocator.getPeakUsage();
    EXPECT_GE(peak2, 300);

    // Reset doesn't change peak
    allocator.reset();
    EXPECT_EQ(allocator.getPeakUsage(), peak2);

    // Allocate less than peak
    void* ptr3 = allocator.allocate(50, 8);
    ASSERT_NE(ptr3, nullptr);
    EXPECT_EQ(allocator.getPeakUsage(), peak2);

    // Allocate more than peak
    void* ptr4 = allocator.allocate(400, 8);
    ASSERT_NE(ptr4, nullptr);
    EXPECT_GE(allocator.getPeakUsage(), 450);
}

TEST(LinearAllocatorTest, AllocationCount) {
    LinearAllocator allocator(1024);

    EXPECT_EQ(allocator.getAllocationCount(), 0);

    allocator.allocate(10, 8);
    EXPECT_EQ(allocator.getAllocationCount(), 1);

    allocator.allocate(20, 8);
    EXPECT_EQ(allocator.getAllocationCount(), 2);

    allocator.reset();
    // Reset doesn't change allocation count
    EXPECT_EQ(allocator.getAllocationCount(), 2);

    allocator.allocate(30, 8);
    EXPECT_EQ(allocator.getAllocationCount(), 3);
}

TEST(LinearAllocatorTest, OwnershipCheck) {
    LinearAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_TRUE(allocator.owns(ptr1));

    // External pointer
    int external = 42;
    EXPECT_FALSE(allocator.owns(&external));

    // Null pointer
    EXPECT_FALSE(allocator.owns(nullptr));
}

TEST(LinearAllocatorTest, DeallocateIsNoop) {
    LinearAllocator allocator(1024);

    void* ptr = allocator.allocate(100, 8);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), 100);

    // Deallocate should be no-op
    allocator.deallocate(ptr, 100);
    EXPECT_EQ(allocator.getAllocatedSize(), 100);
}

TEST(LinearAllocatorTest, ResetStatistics) {
    LinearAllocator allocator(1024);

    allocator.allocate(100, 8);
    allocator.allocate(200, 8);
    EXPECT_EQ(allocator.getAllocationCount(), 2);
    size_t peakBefore = allocator.getPeakUsage();
    EXPECT_GE(peakBefore, 300);

    size_t currentOffset = allocator.getAllocatedSize();
    allocator.resetStatistics();
    EXPECT_EQ(allocator.getAllocationCount(), 0);
    EXPECT_EQ(allocator.getPeakUsage(), currentOffset);  // Peak set to current offset
}

TEST(LinearAllocatorTest, CreateDestroy) {
    LinearAllocator allocator(1024);

    TestObject::resetCounters();

    // Create objects
    TestObject* obj1 = allocator.create<TestObject>(42);
    TestObject* obj2 = allocator.create<TestObject>(100);

    ASSERT_NE(obj1, nullptr);
    ASSERT_NE(obj2, nullptr);
    EXPECT_EQ(obj1->value, 42);
    EXPECT_EQ(obj2->value, 100);
    EXPECT_EQ(TestObject::constructCount, 2);

    // Destroy objects manually
    allocator.destroy(obj1);
    allocator.destroy(obj2);
    EXPECT_EQ(TestObject::destructCount, 2);
}

TEST(LinearAllocatorTest, AllocateArray) {
    LinearAllocator allocator(1024);

    float* floats = allocator.allocateArray<float>(100);
    ASSERT_NE(floats, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), 100 * sizeof(float));

    // Write and read data
    for (int i = 0; i < 100; ++i) {
        floats[i] = static_cast<float>(i);
    }
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(floats[i], static_cast<float>(i));
    }
}

TEST(LinearAllocatorTest, InvalidAlignment) {
    LinearAllocator allocator(1024);

    // Non-power-of-2 alignment should fail
    void* ptr = allocator.allocate(100, 7);
    EXPECT_EQ(ptr, nullptr);
}

TEST(LinearAllocatorTest, ZeroSizeAllocation) {
    LinearAllocator allocator(1024);

    void* ptr = allocator.allocate(0, 8);
    EXPECT_EQ(ptr, nullptr);
}

// ============================================================================
// LinearAllocatorScope tests
// ============================================================================

TEST(LinearAllocatorScopeTest, AutomaticReset) {
    LinearAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), 100);

    size_t markerBeforeScope = allocator.getAllocatedSize();

    {
        LinearAllocatorScope scope(allocator);
        void* ptr2 = allocator.allocate(200, 8);
        void* ptr3 = allocator.allocate(300, 8);
        ASSERT_NE(ptr2, nullptr);
        ASSERT_NE(ptr3, nullptr);
        EXPECT_GE(allocator.getAllocatedSize(), 600);
    }  // scope ends - should reset to marker

    // Should be back to state before scope
    EXPECT_EQ(allocator.getAllocatedSize(), markerBeforeScope);
}

TEST(LinearAllocatorScopeTest, NestedScopes) {
    LinearAllocator allocator(1024);

    EXPECT_EQ(allocator.getAllocatedSize(), 0);

    {
        LinearAllocatorScope scope1(allocator);
        allocator.allocate(100, 8);
        size_t marker1 = allocator.getAllocatedSize();
        EXPECT_EQ(marker1, 100);

        {
            LinearAllocatorScope scope2(allocator);
            allocator.allocate(200, 8);
            size_t marker2 = allocator.getAllocatedSize();
            EXPECT_GE(marker2, 300);

            {
                LinearAllocatorScope scope3(allocator);
                allocator.allocate(300, 8);
                EXPECT_GE(allocator.getAllocatedSize(), 600);
            }  // scope3 ends

            EXPECT_EQ(allocator.getAllocatedSize(), marker2);
        }  // scope2 ends

        EXPECT_EQ(allocator.getAllocatedSize(), marker1);
    }  // scope1 ends

    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

// ============================================================================
// FrameAllocator tests
// ============================================================================

TEST(FrameAllocatorTest, DefaultConstruction) {
    FrameAllocator allocator(2048);

    EXPECT_EQ(allocator.getBufferCapacity(), 1024);  // Split between 2 buffers
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
    EXPECT_EQ(allocator.getFrameNumber(), 0);
    EXPECT_EQ(allocator.remaining(), 1024);
}

TEST(FrameAllocatorTest, SingleFrameAllocation) {
    FrameAllocator allocator(2048);

    void* ptr1 = allocator.allocate(100, 8);
    void* ptr2 = allocator.allocate(200, 8);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_GE(allocator.getAllocatedSize(), 300);
}

TEST(FrameAllocatorTest, FlipSwitchesBuffers) {
    FrameAllocator allocator(2048);

    // Frame 0: Allocate from buffer 0
    void* ptr1 = allocator.allocate(100, 8);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), 100);
    EXPECT_EQ(allocator.getFrameNumber(), 0);

    // Flip to frame 1
    allocator.flip();
    EXPECT_EQ(allocator.getFrameNumber(), 1);
    EXPECT_EQ(allocator.getAllocatedSize(), 100);  // Buffer 0 still has data

    // Frame 1: Allocate from buffer 1
    void* ptr2 = allocator.allocate(200, 8);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), 300);  // 100 + 200

    // Flip to frame 2 - resets buffer 0
    allocator.flip();
    EXPECT_EQ(allocator.getFrameNumber(), 2);
    EXPECT_EQ(allocator.getAllocatedSize(), 200);  // Only buffer 1 has data
}

TEST(FrameAllocatorTest, DoubleBuffering) {
    FrameAllocator allocator(2048);

    // Frame 0
    void* frame0_ptr1 = allocator.allocate(100, 8);
    void* frame0_ptr2 = allocator.allocate(150, 8);
    ASSERT_NE(frame0_ptr1, nullptr);
    ASSERT_NE(frame0_ptr2, nullptr);
    size_t frame0Size = allocator.getAllocatedSize();

    allocator.flip();  // Frame 1

    // Frame 1
    void* frame1_ptr1 = allocator.allocate(200, 8);
    void* frame1_ptr2 = allocator.allocate(250, 8);
    ASSERT_NE(frame1_ptr1, nullptr);
    ASSERT_NE(frame1_ptr2, nullptr);
    size_t frame1Size = allocator.getAllocatedSize() - frame0Size;

    allocator.flip();  // Frame 2 - buffer 0 is reset

    // Frame 2 - using buffer 0 again (reset)
    void* frame2_ptr1 = allocator.allocate(300, 8);
    ASSERT_NE(frame2_ptr1, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), frame1Size + 300);

    allocator.flip();  // Frame 3 - buffer 1 is reset

    // Frame 3 - using buffer 1 again (reset)
    void* frame3_ptr1 = allocator.allocate(350, 8);
    ASSERT_NE(frame3_ptr1, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), 650);  // Only frame 2 + frame 3
}

TEST(FrameAllocatorTest, PeakUsageTracking) {
    FrameAllocator allocator(2048);

    // Frame 0: 300 bytes
    allocator.allocate(300, 8);
    allocator.flip();

    // Frame 1: 500 bytes (new peak)
    allocator.allocate(500, 8);
    EXPECT_EQ(allocator.getPeakUsage(), 500);

    allocator.flip();

    // Frame 2: 200 bytes (less than peak)
    allocator.allocate(200, 8);
    EXPECT_EQ(allocator.getPeakUsage(), 500);
}

TEST(FrameAllocatorTest, OutOfMemoryPerBuffer) {
    FrameAllocator allocator(200);  // 100 bytes per buffer

    // Allocate 80 bytes - should succeed
    void* ptr1 = allocator.allocate(80, 8);
    ASSERT_NE(ptr1, nullptr);

    // Try to allocate 30 more - should fail (only ~20 bytes left)
    void* ptr2 = allocator.allocate(30, 8);
    EXPECT_EQ(ptr2, nullptr);
}

TEST(FrameAllocatorTest, DeallocateIsNoop) {
    FrameAllocator allocator(2048);

    void* ptr = allocator.allocate(100, 8);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), 100);

    // Deallocate should be no-op
    allocator.deallocate(ptr, 100);
    EXPECT_EQ(allocator.getAllocatedSize(), 100);
}

// ============================================================================
// Performance characteristic tests
// ============================================================================

TEST(LinearAllocatorTest, ManySmallAllocations) {
    LinearAllocator allocator(1024 * 1024);  // 1MB

    const size_t count = 10000;
    std::vector<void*> ptrs;

    for (size_t i = 0; i < count; ++i) {
        void* ptr = allocator.allocate(16, 8);
        if (ptr) {
            ptrs.push_back(ptr);
        }
    }

    EXPECT_GT(ptrs.size(), 0);
    EXPECT_EQ(allocator.getAllocationCount(), ptrs.size());
}

TEST(LinearAllocatorTest, LargeAllocation) {
    LinearAllocator allocator(10 * 1024 * 1024);  // 10MB

    // Allocate 8MB block
    void* ptr = allocator.allocate(8 * 1024 * 1024, 64);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 64, 0);
}
