#include "axiom/memory/pool_allocator.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <set>
#include <vector>

using namespace axiom::memory;

// ============================================================================
// Test types
// ============================================================================

struct SmallObject {
    int data[4];  // 16 bytes
};

struct LargeObject {
    double data[16];  // 128 bytes
};

struct AlignedObject {
    alignas(32) float data[8];  // 32 bytes, 32-byte aligned
};

class TrackedObject {
public:
    TrackedObject() : value_(0), padding_{0} { ++constructCount_; }
    explicit TrackedObject(int value) : value_(value), padding_{0} { ++constructCount_; }
    ~TrackedObject() { ++destructCount_; }

    int getValue() const { return value_; }
    void setValue(int value) { value_ = value; }

    static void resetCounters() {
        constructCount_ = 0;
        destructCount_ = 0;
    }

    static int constructCount_;
    static int destructCount_;

private:
    int value_;
    [[maybe_unused]] int padding_;  // Ensure size >= sizeof(void*)
};

int TrackedObject::constructCount_ = 0;
int TrackedObject::destructCount_ = 0;

// ============================================================================
// Basic allocation tests
// ============================================================================

TEST(PoolAllocatorTest, DefaultConstruction) {
    PoolAllocator<64, 8> pool;
    EXPECT_EQ(pool.size(), 0);
    EXPECT_EQ(pool.capacity(), 0);
    EXPECT_EQ(pool.getAllocatedSize(), 0);
}

TEST(PoolAllocatorTest, SingleAllocation) {
    PoolAllocator<64, 8> pool;

    void* ptr = pool.allocate(64, 8);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(pool.size(), 1);
    EXPECT_GT(pool.capacity(), 0);
    EXPECT_TRUE(pool.owns(ptr));

    // Check alignment
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 8, 0);

    pool.deallocate(ptr, 64);
    EXPECT_EQ(pool.size(), 0);
}

TEST(PoolAllocatorTest, MultipleAllocations) {
    PoolAllocator<32, 8> pool;

    std::vector<void*> ptrs;
    const size_t count = 10;

    for (size_t i = 0; i < count; ++i) {
        void* ptr = pool.allocate(32, 8);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    EXPECT_EQ(pool.size(), count);
    EXPECT_GE(pool.capacity(), count);

    // All pointers should be unique
    std::set<void*> uniquePtrs(ptrs.begin(), ptrs.end());
    EXPECT_EQ(uniquePtrs.size(), count);

    // All pointers should be owned by the pool
    for (void* ptr : ptrs) {
        EXPECT_TRUE(pool.owns(ptr));
    }

    // Deallocate all
    for (void* ptr : ptrs) {
        pool.deallocate(ptr, 32);
    }

    EXPECT_EQ(pool.size(), 0);
}

TEST(PoolAllocatorTest, AllocationWithWrongSize) {
    PoolAllocator<64, 8> pool;

    void* ptr1 = pool.allocate(32, 8);  // Wrong size
    EXPECT_EQ(ptr1, nullptr);

    void* ptr2 = pool.allocate(64, 16);  // Wrong alignment
    EXPECT_EQ(ptr2, nullptr);
}

TEST(PoolAllocatorTest, DeallocationWithWrongSize) {
    PoolAllocator<64, 8> pool;

    void* ptr = pool.allocate(64, 8);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(pool.size(), 1);

    // Deallocate with wrong size (should be ignored)
    pool.deallocate(ptr, 32);
    EXPECT_EQ(pool.size(), 1);  // Size unchanged

    // Deallocate with correct size
    pool.deallocate(ptr, 64);
    EXPECT_EQ(pool.size(), 0);
}

TEST(PoolAllocatorTest, DeallocateNullptr) {
    PoolAllocator<64, 8> pool;

    // Should not crash
    pool.deallocate(nullptr, 64);
    EXPECT_EQ(pool.size(), 0);
}

// ============================================================================
// Free-list reuse tests
// ============================================================================

TEST(PoolAllocatorTest, FreeListReuse) {
    PoolAllocator<64, 8> pool;

    // Allocate
    void* ptr1 = pool.allocate(64, 8);
    ASSERT_NE(ptr1, nullptr);

    // Deallocate (returns to free-list)
    pool.deallocate(ptr1, 64);
    EXPECT_EQ(pool.size(), 0);

    // Allocate again (should reuse same block)
    void* ptr2 = pool.allocate(64, 8);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(ptr1, ptr2);  // Same pointer reused
    EXPECT_EQ(pool.size(), 1);

    pool.deallocate(ptr2, 64);
}

TEST(PoolAllocatorTest, MultipleReusePattern) {
    PoolAllocator<32, 8> pool;

    std::vector<void*> ptrs;
    const size_t count = 20;

    // Allocate many blocks
    for (size_t i = 0; i < count; ++i) {
        void* ptr = pool.allocate(32, 8);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    size_t initialCapacity = pool.capacity();
    EXPECT_EQ(pool.size(), count);

    // Deallocate all
    for (void* ptr : ptrs) {
        pool.deallocate(ptr, 32);
    }
    EXPECT_EQ(pool.size(), 0);

    // Allocate again (should reuse without growing)
    for (size_t i = 0; i < count; ++i) {
        void* ptr = pool.allocate(32, 8);
        ASSERT_NE(ptr, nullptr);
        pool.deallocate(ptr, 32);
    }

    EXPECT_EQ(pool.capacity(), initialCapacity);  // No growth
}

TEST(PoolAllocatorTest, InterleavedAllocateDealloc) {
    PoolAllocator<64, 8> pool;

    void* ptr1 = pool.allocate(64, 8);
    void* ptr2 = pool.allocate(64, 8);
    void* ptr3 = pool.allocate(64, 8);

    EXPECT_EQ(pool.size(), 3);

    pool.deallocate(ptr2, 64);  // Deallocate middle
    EXPECT_EQ(pool.size(), 2);

    void* ptr4 = pool.allocate(64, 8);  // Should reuse ptr2
    EXPECT_EQ(ptr4, ptr2);
    EXPECT_EQ(pool.size(), 3);

    pool.deallocate(ptr1, 64);
    pool.deallocate(ptr3, 64);
    pool.deallocate(ptr4, 64);
}

// ============================================================================
// Capacity and reserve tests
// ============================================================================

TEST(PoolAllocatorTest, CustomBlocksPerChunk) {
    PoolAllocator<64, 8> pool(128);  // 128 blocks per chunk

    // Allocate one block (triggers chunk allocation)
    void* ptr = pool.allocate(64, 8);
    ASSERT_NE(ptr, nullptr);

    EXPECT_GE(pool.capacity(), 128);
    EXPECT_EQ(pool.size(), 1);

    pool.deallocate(ptr, 64);
}

TEST(PoolAllocatorTest, ReserveCapacity) {
    PoolAllocator<64, 8> pool(64);  // 64 blocks per chunk

    // Reserve space for 200 blocks
    pool.reserve(200);

    EXPECT_GE(pool.capacity(), 200);
    EXPECT_EQ(pool.size(), 0);  // Size is still 0

    // Allocate 200 blocks (should not trigger new chunk allocations)
    size_t capacityBefore = pool.capacity();
    for (size_t i = 0; i < 200; ++i) {
        void* ptr = pool.allocate(64, 8);
        ASSERT_NE(ptr, nullptr);
        pool.deallocate(ptr, 64);
    }

    EXPECT_EQ(pool.capacity(), capacityBefore);  // No growth
}

TEST(PoolAllocatorTest, ReserveAlreadySufficient) {
    PoolAllocator<64, 8> pool(256);  // 256 blocks per chunk

    void* ptr = pool.allocate(64, 8);  // Allocate one to create chunk
    ASSERT_NE(ptr, nullptr);

    size_t capacityBefore = pool.capacity();
    EXPECT_GE(capacityBefore, 256);

    // Reserve less than current capacity (no-op)
    pool.reserve(100);
    EXPECT_EQ(pool.capacity(), capacityBefore);

    pool.deallocate(ptr, 64);
}

TEST(PoolAllocatorTest, ChunkGrowth) {
    PoolAllocator<64, 8> pool(16);  // Small chunk size

    std::vector<void*> ptrs;

    // Allocate more than one chunk worth
    for (size_t i = 0; i < 50; ++i) {
        void* ptr = pool.allocate(64, 8);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    EXPECT_GE(pool.capacity(), 50);
    EXPECT_EQ(pool.size(), 50);

    // Clean up
    for (void* ptr : ptrs) {
        pool.deallocate(ptr, 64);
    }
}

// ============================================================================
// owns() method tests
// ============================================================================

TEST(PoolAllocatorTest, OwnsValidPointers) {
    PoolAllocator<64, 8> pool;

    void* ptr1 = pool.allocate(64, 8);
    void* ptr2 = pool.allocate(64, 8);

    EXPECT_TRUE(pool.owns(ptr1));
    EXPECT_TRUE(pool.owns(ptr2));

    pool.deallocate(ptr1, 64);

    // Still owns deallocated pointer (it's in the free-list)
    EXPECT_TRUE(pool.owns(ptr1));
    EXPECT_TRUE(pool.owns(ptr2));

    pool.deallocate(ptr2, 64);
}

TEST(PoolAllocatorTest, OwnsInvalidPointers) {
    PoolAllocator<64, 8> pool1;
    PoolAllocator<64, 8> pool2;

    void* ptr1 = pool1.allocate(64, 8);
    void* ptr2 = pool2.allocate(64, 8);

    EXPECT_TRUE(pool1.owns(ptr1));
    EXPECT_FALSE(pool1.owns(ptr2));  // From different pool

    EXPECT_FALSE(pool2.owns(ptr1));
    EXPECT_TRUE(pool2.owns(ptr2));

    pool1.deallocate(ptr1, 64);
    pool2.deallocate(ptr2, 64);
}

TEST(PoolAllocatorTest, OwnsNullptr) {
    PoolAllocator<64, 8> pool;

    EXPECT_FALSE(pool.owns(nullptr));
}

TEST(PoolAllocatorTest, OwnsArbitraryPointer) {
    PoolAllocator<64, 8> pool;

    int stackVar = 42;
    EXPECT_FALSE(pool.owns(&stackVar));

    int* heapVar = new int(100);
    EXPECT_FALSE(pool.owns(heapVar));
    delete heapVar;
}

// ============================================================================
// clear() tests
// ============================================================================

TEST(PoolAllocatorTest, ClearEmptyPool) {
    PoolAllocator<64, 8> pool;

    pool.clear();

    EXPECT_EQ(pool.size(), 0);
    EXPECT_EQ(pool.capacity(), 0);
    EXPECT_EQ(pool.getAllocatedSize(), 0);
}

TEST(PoolAllocatorTest, ClearAfterAllocations) {
    PoolAllocator<64, 8> pool;

    // Allocate some blocks
    std::vector<void*> ptrs;
    for (int i = 0; i < 10; ++i) {
        void* ptr = pool.allocate(64, 8);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    EXPECT_GT(pool.capacity(), 0);
    EXPECT_EQ(pool.size(), 10);

    // Clear
    pool.clear();

    EXPECT_EQ(pool.size(), 0);
    EXPECT_EQ(pool.capacity(), 0);
    EXPECT_EQ(pool.getAllocatedSize(), 0);

    // Old pointers should no longer be owned
    for (void* ptr : ptrs) {
        EXPECT_FALSE(pool.owns(ptr));
    }
}

TEST(PoolAllocatorTest, AllocateAfterClear) {
    PoolAllocator<64, 8> pool;

    void* ptr1 = pool.allocate(64, 8);
    ASSERT_NE(ptr1, nullptr);

    pool.clear();

    // Allocate after clear (should work)
    void* ptr2 = pool.allocate(64, 8);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(pool.size(), 1);

    pool.deallocate(ptr2, 64);
}

// ============================================================================
// Alignment tests
// ============================================================================

TEST(PoolAllocatorTest, DefaultAlignment) {
    PoolAllocator<64, 16> pool;

    void* ptr = pool.allocate(64, 16);
    ASSERT_NE(ptr, nullptr);

    // Check 16-byte alignment
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 16, 0);

    pool.deallocate(ptr, 64);
}

TEST(PoolAllocatorTest, HighAlignment) {
    PoolAllocator<64, 64> pool;

    void* ptr = pool.allocate(64, 64);
    ASSERT_NE(ptr, nullptr);

    // Check 64-byte alignment (cache-line)
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 64, 0);

    pool.deallocate(ptr, 64);
}

TEST(PoolAllocatorTest, AllBlocksAligned) {
    PoolAllocator<128, 32> pool;

    std::vector<void*> ptrs;
    for (int i = 0; i < 20; ++i) {
        void* ptr = pool.allocate(128, 32);
        ASSERT_NE(ptr, nullptr);

        // Every block should be 32-byte aligned
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 32, 0);

        ptrs.push_back(ptr);
    }

    for (void* ptr : ptrs) {
        pool.deallocate(ptr, 128);
    }
}

// ============================================================================
// Object construction/destruction tests
// ============================================================================

TEST(PoolAllocatorTest, PlacementNewDelete) {
    TrackedObject::resetCounters();

    PoolAllocator<sizeof(TrackedObject), alignof(TrackedObject)> pool;

    // Allocate and construct
    void* ptr = pool.allocate(sizeof(TrackedObject), alignof(TrackedObject));
    ASSERT_NE(ptr, nullptr);

    TrackedObject* obj = new (ptr) TrackedObject(42);
    EXPECT_EQ(TrackedObject::constructCount_, 1);
    EXPECT_EQ(obj->getValue(), 42);

    // Modify
    obj->setValue(100);
    EXPECT_EQ(obj->getValue(), 100);

    // Destroy and deallocate
    obj->~TrackedObject();
    EXPECT_EQ(TrackedObject::destructCount_, 1);

    pool.deallocate(ptr, sizeof(TrackedObject));
}

TEST(PoolAllocatorTest, MultipleObjectLifecycles) {
    TrackedObject::resetCounters();

    PoolAllocator<sizeof(TrackedObject), alignof(TrackedObject)> pool;

    const int iterations = 50;
    for (int i = 0; i < iterations; ++i) {
        void* ptr = pool.allocate(sizeof(TrackedObject), alignof(TrackedObject));
        ASSERT_NE(ptr, nullptr);

        TrackedObject* obj = new (ptr) TrackedObject(i);
        EXPECT_EQ(obj->getValue(), i);

        obj->~TrackedObject();
        pool.deallocate(ptr, sizeof(TrackedObject));
    }

    EXPECT_EQ(TrackedObject::constructCount_, iterations);
    EXPECT_EQ(TrackedObject::destructCount_, iterations);
}

// ============================================================================
// Memory operations tests
// ============================================================================

TEST(PoolAllocatorTest, ReadWriteMemory) {
    PoolAllocator<256, 8> pool;

    void* ptr = pool.allocate(256, 8);
    ASSERT_NE(ptr, nullptr);

    // Write pattern
    uint8_t* bytes = static_cast<uint8_t*>(ptr);
    for (size_t i = 0; i < 256; ++i) {
        bytes[i] = static_cast<uint8_t>(i);
    }

    // Read and verify
    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(bytes[i], static_cast<uint8_t>(i));
    }

    pool.deallocate(ptr, 256);
}

TEST(PoolAllocatorTest, MemoryIndependence) {
    PoolAllocator<64, 8> pool;

    void* ptr1 = pool.allocate(64, 8);
    void* ptr2 = pool.allocate(64, 8);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    // Write different patterns
    std::memset(ptr1, 0xAA, 64);
    std::memset(ptr2, 0x55, 64);

    // Verify independence
    uint8_t* bytes1 = static_cast<uint8_t*>(ptr1);
    uint8_t* bytes2 = static_cast<uint8_t*>(ptr2);

    for (size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(bytes1[i], 0xAA);
        EXPECT_EQ(bytes2[i], 0x55);
    }

    pool.deallocate(ptr1, 64);
    pool.deallocate(ptr2, 64);
}

// ============================================================================
// getAllocatedSize() tests
// ============================================================================

TEST(PoolAllocatorTest, AllocatedSizeTracking) {
    PoolAllocator<64, 8> pool(16);  // 16 blocks per chunk

    EXPECT_EQ(pool.getAllocatedSize(), 0);

    // Allocate one block (triggers chunk allocation)
    void* ptr1 = pool.allocate(64, 8);
    ASSERT_NE(ptr1, nullptr);

    // Allocated size is full chunk, not just one block
    EXPECT_EQ(pool.getAllocatedSize(), 16 * 64);

    // Allocate more blocks (same chunk)
    void* ptr2 = pool.allocate(64, 8);
    EXPECT_EQ(pool.getAllocatedSize(), 16 * 64);

    // Deallocate (doesn't change allocated size)
    pool.deallocate(ptr1, 64);
    EXPECT_EQ(pool.getAllocatedSize(), 16 * 64);

    pool.deallocate(ptr2, 64);
    EXPECT_EQ(pool.getAllocatedSize(), 16 * 64);

    // Clear frees everything
    pool.clear();
    EXPECT_EQ(pool.getAllocatedSize(), 0);
}

// ============================================================================
// Size and capacity edge cases
// ============================================================================

TEST(PoolAllocatorTest, SizeCapacityConsistency) {
    PoolAllocator<64, 8> pool(32);

    EXPECT_EQ(pool.size(), 0);
    EXPECT_EQ(pool.capacity(), 0);

    // Allocate 10 blocks
    std::vector<void*> ptrs;
    for (int i = 0; i < 10; ++i) {
        void* ptr = pool.allocate(64, 8);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    EXPECT_EQ(pool.size(), 10);
    EXPECT_GE(pool.capacity(), 32);  // At least one chunk

    // Deallocate 5 blocks
    for (size_t i = 0; i < 5; ++i) {
        pool.deallocate(ptrs[i], 64);
    }

    EXPECT_EQ(pool.size(), 5);
    EXPECT_GE(pool.capacity(), 32);  // Capacity unchanged

    // Deallocate remaining
    for (size_t i = 5; i < 10; ++i) {
        pool.deallocate(ptrs[i], 64);
    }

    EXPECT_EQ(pool.size(), 0);
    EXPECT_GE(pool.capacity(), 32);  // Capacity unchanged
}

// ============================================================================
// Stress tests
// ============================================================================

TEST(PoolAllocatorTest, ManyAllocations) {
    PoolAllocator<64, 8> pool(128);

    std::vector<void*> ptrs;
    const size_t count = 1000;

    // Allocate many blocks
    for (size_t i = 0; i < count; ++i) {
        void* ptr = pool.allocate(64, 8);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    EXPECT_EQ(pool.size(), count);
    EXPECT_GE(pool.capacity(), count);

    // All should be unique
    std::set<void*> uniquePtrs(ptrs.begin(), ptrs.end());
    EXPECT_EQ(uniquePtrs.size(), count);

    // Deallocate all
    for (void* ptr : ptrs) {
        pool.deallocate(ptr, 64);
    }

    EXPECT_EQ(pool.size(), 0);
}

TEST(PoolAllocatorTest, RandomAllocationPattern) {
    PoolAllocator<32, 8> pool(64);

    std::vector<void*> ptrs;
    ptrs.reserve(100);

    // Randomly allocate and deallocate
    for (size_t iteration = 0; iteration < 500; ++iteration) {
        if (ptrs.empty() || (ptrs.size() < 100 && iteration % 3 != 0)) {
            // Allocate
            void* ptr = pool.allocate(32, 8);
            ASSERT_NE(ptr, nullptr);
            ptrs.push_back(ptr);
        } else {
            // Deallocate random element
            size_t index = iteration % ptrs.size();
            pool.deallocate(ptrs[index], 32);
            ptrs.erase(ptrs.begin() + static_cast<std::ptrdiff_t>(index));
        }
    }

    // Clean up remaining
    for (void* ptr : ptrs) {
        pool.deallocate(ptr, 32);
    }

    EXPECT_EQ(pool.size(), 0);
}

// ============================================================================
// Allocator interface compatibility
// ============================================================================

TEST(PoolAllocatorTest, AllocatorInterface) {
    PoolAllocator<64, 8> pool;

    // Use through Allocator interface
    Allocator* allocator = &pool;

    void* ptr = allocator->allocate(64, 8);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(allocator->getAllocatedSize(), pool.getAllocatedSize());

    allocator->deallocate(ptr, 64);
}
