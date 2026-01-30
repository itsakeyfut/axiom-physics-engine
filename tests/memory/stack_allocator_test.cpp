#include "axiom/memory/stack_allocator.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

using namespace axiom::memory;

// ============================================================================
// Test utilities
// ============================================================================

namespace {

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

}  // anonymous namespace

// ============================================================================
// StackAllocator tests
// ============================================================================

TEST(StackAllocatorTest, DefaultConstruction) {
    StackAllocator allocator(1024);
    EXPECT_EQ(allocator.capacity(), 1024);
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
    EXPECT_EQ(allocator.remaining(), 1024);
    EXPECT_EQ(allocator.getPeakUsage(), 0);
    EXPECT_EQ(allocator.getAllocationCount(), 0);
    EXPECT_EQ(allocator.getDeallocationCount(), 0);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 0);
}

TEST(StackAllocatorTest, ZeroCapacity) {
    StackAllocator allocator(0);
    EXPECT_EQ(allocator.capacity(), 0);
    EXPECT_EQ(allocator.remaining(), 0);

    void* ptr = allocator.allocate(100, 8);
    EXPECT_EQ(ptr, nullptr);
}

TEST(StackAllocatorTest, SingleAllocation) {
    StackAllocator allocator(1024);

    void* ptr = allocator.allocate(100, 8);
    ASSERT_NE(ptr, nullptr);
    EXPECT_GT(allocator.getAllocatedSize(), 100);  // Includes header overhead
    EXPECT_EQ(allocator.getAllocationCount(), 1);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 1);
    EXPECT_TRUE(allocator.owns(ptr));
}

TEST(StackAllocatorTest, MultipleAllocations) {
    StackAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    void* ptr2 = allocator.allocate(200, 8);
    void* ptr3 = allocator.allocate(300, 8);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);

    // Size includes header overhead and alignment padding
    EXPECT_GE(allocator.getAllocatedSize(), 600);
    EXPECT_EQ(allocator.getAllocationCount(), 3);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 3);

    // Pointers should be owned by allocator
    EXPECT_TRUE(allocator.owns(ptr1));
    EXPECT_TRUE(allocator.owns(ptr2));
    EXPECT_TRUE(allocator.owns(ptr3));
}

TEST(StackAllocatorTest, AlignmentVerification) {
    StackAllocator allocator(1024);

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

TEST(StackAllocatorTest, LIFODeallocation) {
    StackAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    void* ptr2 = allocator.allocate(200, 8);
    void* ptr3 = allocator.allocate(300, 8);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);

    EXPECT_EQ(allocator.getActiveAllocationCount(), 3);

    // Deallocate in LIFO order
    allocator.deallocate(ptr3, 300);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 2);
    EXPECT_EQ(allocator.getDeallocationCount(), 1);

    allocator.deallocate(ptr2, 200);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 1);
    EXPECT_EQ(allocator.getDeallocationCount(), 2);

    allocator.deallocate(ptr1, 100);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 0);
    EXPECT_EQ(allocator.getDeallocationCount(), 3);
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

TEST(StackAllocatorTest, LIFOReallocation) {
    StackAllocator allocator(1024);

    // Allocate, deallocate, reallocate
    void* ptr1 = allocator.allocate(100, 8);
    ASSERT_NE(ptr1, nullptr);

    allocator.deallocate(ptr1, 100);
    EXPECT_EQ(allocator.getAllocatedSize(), 0);

    // Should be able to allocate again
    void* ptr2 = allocator.allocate(200, 8);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 1);
}

TEST(StackAllocatorTest, OutOfMemory) {
    StackAllocator allocator(200);

    void* ptr1 = allocator.allocate(50, 8);
    ASSERT_NE(ptr1, nullptr);

    void* ptr2 = allocator.allocate(60, 8);
    ASSERT_NE(ptr2, nullptr);

    // This should fail - not enough space (including header overhead)
    void* ptr3 = allocator.allocate(100, 8);
    EXPECT_EQ(ptr3, nullptr);
}

TEST(StackAllocatorTest, Reset) {
    StackAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    void* ptr2 = allocator.allocate(200, 8);
    void* ptr3 = allocator.allocate(300, 8);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);

    size_t sizeBeforeReset = allocator.getAllocatedSize();
    EXPECT_GT(sizeBeforeReset, 600);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 3);

    allocator.reset();
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
    EXPECT_EQ(allocator.remaining(), 1024);

    // Should be able to allocate again
    void* ptr4 = allocator.allocate(500, 8);
    ASSERT_NE(ptr4, nullptr);
    EXPECT_GT(allocator.getAllocatedSize(), 500);
}

TEST(StackAllocatorTest, PeakUsageTracking) {
    StackAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    ASSERT_NE(ptr1, nullptr);
    size_t peak1 = allocator.getPeakUsage();
    EXPECT_GT(peak1, 100);

    void* ptr2 = allocator.allocate(200, 8);
    ASSERT_NE(ptr2, nullptr);
    size_t peak2 = allocator.getPeakUsage();
    EXPECT_GT(peak2, 300);
    EXPECT_GT(peak2, peak1);

    // Deallocate doesn't change peak
    allocator.deallocate(ptr2, 200);
    EXPECT_EQ(allocator.getPeakUsage(), peak2);

    allocator.deallocate(ptr1, 100);
    EXPECT_EQ(allocator.getPeakUsage(), peak2);

    // Allocate less than peak
    void* ptr3 = allocator.allocate(50, 8);
    ASSERT_NE(ptr3, nullptr);
    EXPECT_EQ(allocator.getPeakUsage(), peak2);

    // Allocate more than peak
    void* ptr4 = allocator.allocate(400, 8);
    ASSERT_NE(ptr4, nullptr);
    size_t peak3 = allocator.getPeakUsage();
    EXPECT_GT(peak3, peak2);
}

TEST(StackAllocatorTest, AllocationCounts) {
    StackAllocator allocator(1024);

    EXPECT_EQ(allocator.getAllocationCount(), 0);
    EXPECT_EQ(allocator.getDeallocationCount(), 0);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 0);

    void* ptr1 = allocator.allocate(10, 8);
    EXPECT_EQ(allocator.getAllocationCount(), 1);
    EXPECT_EQ(allocator.getDeallocationCount(), 0);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 1);

    void* ptr2 = allocator.allocate(20, 8);
    EXPECT_EQ(allocator.getAllocationCount(), 2);
    EXPECT_EQ(allocator.getDeallocationCount(), 0);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 2);

    allocator.deallocate(ptr2, 20);
    EXPECT_EQ(allocator.getAllocationCount(), 2);
    EXPECT_EQ(allocator.getDeallocationCount(), 1);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 1);

    allocator.deallocate(ptr1, 10);
    EXPECT_EQ(allocator.getAllocationCount(), 2);
    EXPECT_EQ(allocator.getDeallocationCount(), 2);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 0);
}

TEST(StackAllocatorTest, OwnershipCheck) {
    StackAllocator allocator(1024);

    void* ptr1 = allocator.allocate(100, 8);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_TRUE(allocator.owns(ptr1));

    // External pointer
    int external = 42;
    EXPECT_FALSE(allocator.owns(&external));

    // Null pointer
    EXPECT_FALSE(allocator.owns(nullptr));
}

TEST(StackAllocatorTest, ResetStatistics) {
    StackAllocator allocator(1024);

    allocator.allocate(100, 8);
    allocator.allocate(200, 8);

    EXPECT_EQ(allocator.getAllocationCount(), 2);
    EXPECT_EQ(allocator.getDeallocationCount(), 0);
    size_t peakBefore = allocator.getPeakUsage();
    EXPECT_GT(peakBefore, 300);

    size_t currentOffset = allocator.getAllocatedSize();
    allocator.resetStatistics();

    EXPECT_EQ(allocator.getAllocationCount(), 0);
    EXPECT_EQ(allocator.getDeallocationCount(), 0);
    EXPECT_EQ(allocator.getPeakUsage(), currentOffset);
}

TEST(StackAllocatorTest, CreateDestroy) {
    StackAllocator allocator(1024);

    TestObject::resetCounters();

    // Create objects
    TestObject* obj1 = allocator.create<TestObject>(42);
    TestObject* obj2 = allocator.create<TestObject>(100);

    ASSERT_NE(obj1, nullptr);
    ASSERT_NE(obj2, nullptr);
    EXPECT_EQ(obj1->value, 42);
    EXPECT_EQ(obj2->value, 100);
    EXPECT_EQ(TestObject::constructCount, 2);

    // Destroy objects in LIFO order
    allocator.destroy(obj2);
    EXPECT_EQ(TestObject::destructCount, 1);

    allocator.destroy(obj1);
    EXPECT_EQ(TestObject::destructCount, 2);
}

TEST(StackAllocatorTest, AllocateArray) {
    StackAllocator allocator(1024);

    float* floats = allocator.allocateArray<float>(100);
    ASSERT_NE(floats, nullptr);

    // Write and read data
    for (int i = 0; i < 100; ++i) {
        floats[i] = static_cast<float>(i);
    }
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(floats[i], static_cast<float>(i));
    }

    allocator.deallocateArray(floats, 100);
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

TEST(StackAllocatorTest, InvalidAlignment) {
    StackAllocator allocator(1024);

    // Non-power-of-2 alignment should fail
    void* ptr = allocator.allocate(100, 7);
    EXPECT_EQ(ptr, nullptr);
}

TEST(StackAllocatorTest, ZeroSizeAllocation) {
    StackAllocator allocator(1024);

    void* ptr = allocator.allocate(0, 8);
    EXPECT_EQ(ptr, nullptr);
}

TEST(StackAllocatorTest, NullptrDeallocation) {
    StackAllocator allocator(1024);

    // Should be safe to deallocate nullptr
    allocator.deallocate(nullptr, 100);
    EXPECT_EQ(allocator.getDeallocationCount(), 0);
}

// ============================================================================
// StackArray tests
// ============================================================================

TEST(StackArrayTest, BasicUsage) {
    StackAllocator allocator(1024);

    {
        StackArray<float> arr(allocator, 100);
        ASSERT_TRUE(arr.isValid());
        EXPECT_NE(arr.data(), nullptr);
        EXPECT_EQ(arr.size(), 100);

        // Write data
        for (size_t i = 0; i < arr.size(); ++i) {
            arr[i] = static_cast<float>(i);
        }

        // Read data
        for (size_t i = 0; i < arr.size(); ++i) {
            EXPECT_EQ(arr[i], static_cast<float>(i));
        }
    }  // Automatic deallocation

    // Memory should be reclaimed
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

TEST(StackArrayTest, NestedArrays) {
    StackAllocator allocator(1024);

    EXPECT_EQ(allocator.getAllocatedSize(), 0);

    {
        StackArray<float> arr1(allocator, 50);
        ASSERT_TRUE(arr1.isValid());
        size_t size1 = allocator.getAllocatedSize();
        EXPECT_GT(size1, 0);

        {
            StackArray<int> arr2(allocator, 100);
            ASSERT_TRUE(arr2.isValid());
            size_t size2 = allocator.getAllocatedSize();
            EXPECT_GT(size2, size1);

            {
                StackArray<double> arr3(allocator, 25);
                ASSERT_TRUE(arr3.isValid());
                EXPECT_GT(allocator.getAllocatedSize(), size2);
            }  // arr3 deallocated

            EXPECT_EQ(allocator.getAllocatedSize(), size2);
        }  // arr2 deallocated

        EXPECT_EQ(allocator.getAllocatedSize(), size1);
    }  // arr1 deallocated

    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

TEST(StackArrayTest, ZeroSize) {
    StackAllocator allocator(1024);

    StackArray<float> arr(allocator, 0);
    EXPECT_EQ(arr.data(), nullptr);
    EXPECT_EQ(arr.size(), 0);
    EXPECT_FALSE(arr.isValid());
}

TEST(StackArrayTest, AllocationFailure) {
    StackAllocator allocator(100);  // Small allocator

    // Try to allocate more than available
    StackArray<float> arr(allocator, 1000);
    EXPECT_FALSE(arr.isValid());
    EXPECT_EQ(arr.data(), nullptr);
}

TEST(StackArrayTest, ConstAccess) {
    StackAllocator allocator(1024);

    StackArray<int> arr(allocator, 10);
    ASSERT_TRUE(arr.isValid());

    // Fill array
    for (size_t i = 0; i < arr.size(); ++i) {
        arr[i] = static_cast<int>(i * 2);
    }

    // Const access
    const StackArray<int>& constArr = arr;
    for (size_t i = 0; i < constArr.size(); ++i) {
        EXPECT_EQ(constArr[i], static_cast<int>(i * 2));
    }

    const int* constData = constArr.data();
    EXPECT_NE(constData, nullptr);
}

// ============================================================================
// Performance characteristic tests
// ============================================================================

TEST(StackAllocatorTest, ManySmallAllocations) {
    StackAllocator allocator(1024 * 1024);  // 1MB

    const size_t count = 1000;
    std::vector<void*> ptrs;

    // Allocate many small blocks
    for (size_t i = 0; i < count; ++i) {
        void* ptr = allocator.allocate(16, 8);
        if (ptr) {
            ptrs.push_back(ptr);
        }
    }

    EXPECT_GT(ptrs.size(), 0);
    EXPECT_EQ(allocator.getAllocationCount(), ptrs.size());

    // Deallocate in LIFO order
    for (size_t i = ptrs.size(); i > 0; --i) {
        allocator.deallocate(ptrs[i - 1], 16);
    }

    EXPECT_EQ(allocator.getAllocatedSize(), 0);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 0);
}

TEST(StackAllocatorTest, LargeAllocation) {
    StackAllocator allocator(10 * 1024 * 1024);  // 10MB

    // Allocate 8MB block
    void* ptr = allocator.allocate(8 * 1024 * 1024, 64);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 64, 0);

    allocator.deallocate(ptr, 8 * 1024 * 1024);
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

TEST(StackAllocatorTest, AlternatingAllocDealloc) {
    StackAllocator allocator(1024);

    for (int i = 0; i < 100; ++i) {
        void* ptr = allocator.allocate(50, 8);
        ASSERT_NE(ptr, nullptr);

        allocator.deallocate(ptr, 50);
        EXPECT_EQ(allocator.getAllocatedSize(), 0);
    }

    EXPECT_EQ(allocator.getAllocationCount(), 100);
    EXPECT_EQ(allocator.getDeallocationCount(), 100);
    EXPECT_EQ(allocator.getActiveAllocationCount(), 0);
}
