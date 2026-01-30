#include "axiom/memory/allocator.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

using namespace axiom::memory;

// ============================================================================
// Mock Allocator for testing
// ============================================================================

class MockAllocator : public Allocator {
public:
    void* allocate(size_t size, size_t alignment) override {
        allocatedSize_ += size;
        allocations_.push_back({size, alignment});
        return alignedAlloc(size, alignment);
    }

    void deallocate(void* ptr, size_t size) override {
        if (ptr) {
            allocatedSize_ -= size;
            deallocations_.push_back(size);
            alignedFree(ptr);
        }
    }

    size_t getAllocatedSize() const override { return allocatedSize_; }

    size_t getAllocationCount() const { return allocations_.size(); }
    size_t getDeallocationCount() const { return deallocations_.size(); }

    void reset() {
        allocatedSize_ = 0;
        allocations_.clear();
        deallocations_.clear();
    }

    struct AllocationInfo {
        size_t size;
        size_t alignment;
    };

    std::vector<AllocationInfo> allocations_;
    std::vector<size_t> deallocations_;
    size_t allocatedSize_ = 0;
};

// ============================================================================
// Test types
// ============================================================================

struct PODType {
    int x;
    float y;
    double z;
};

class NonPODType {
public:
    NonPODType() : value_(0), constructed_(true) { ++constructorCalls; }

    explicit NonPODType(int value) : value_(value), constructed_(true) { ++constructorCalls; }

    NonPODType(int a, int b) : value_(a + b), constructed_(true) { ++constructorCalls; }

    ~NonPODType() {
        if (constructed_) {
            ++destructorCalls;
            constructed_ = false;
        }
    }

    int getValue() const { return value_; }

    static void resetCounters() {
        constructorCalls = 0;
        destructorCalls = 0;
    }

    static int constructorCalls;
    static int destructorCalls;

private:
    int value_;
    bool constructed_;
};

int NonPODType::constructorCalls = 0;
int NonPODType::destructorCalls = 0;

// Aligned type for SIMD testing
struct alignas(32) AlignedType {
    float data[8];
};

// ============================================================================
// alignedAlloc/alignedFree tests
// ============================================================================

TEST(AlignedAllocTest, BasicAllocation) {
    void* ptr = alignedAlloc(1024, 16);
    ASSERT_NE(ptr, nullptr);

    // Check alignment
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 16, 0);

    alignedFree(ptr);
}

TEST(AlignedAllocTest, VariousAlignments) {
    const size_t alignments[] = {8, 16, 32, 64};

    for (size_t align : alignments) {
        void* ptr = alignedAlloc(256, align);
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % align, 0)
            << "Alignment " << align << " failed";
        alignedFree(ptr);
    }
}

TEST(AlignedAllocTest, ZeroSize) {
    void* ptr = alignedAlloc(0, 16);
    EXPECT_EQ(ptr, nullptr);
}

TEST(AlignedAllocTest, NonPowerOfTwoAlignment) {
    void* ptr = alignedAlloc(256, 15);  // 15 is not a power of 2
    EXPECT_EQ(ptr, nullptr);
}

TEST(AlignedAllocTest, FreeNullptr) {
    // Should not crash
    alignedFree(nullptr);
}

TEST(AlignedAllocTest, ReadWrite) {
    const size_t size = 1024;
    void* ptr = alignedAlloc(size, 32);
    ASSERT_NE(ptr, nullptr);

    // Write pattern
    unsigned char* bytes = static_cast<unsigned char*>(ptr);
    for (size_t i = 0; i < size; ++i) {
        bytes[i] = static_cast<unsigned char>(i % 256);
    }

    // Verify pattern
    for (size_t i = 0; i < size; ++i) {
        EXPECT_EQ(bytes[i], static_cast<unsigned char>(i % 256));
    }

    alignedFree(ptr);
}

// ============================================================================
// Allocator interface tests
// ============================================================================

TEST(AllocatorTest, BasicAllocation) {
    MockAllocator allocator;

    void* ptr = allocator.allocate(1024, 16);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), 1024);
    EXPECT_EQ(allocator.getAllocationCount(), 1);

    allocator.deallocate(ptr, 1024);
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
    EXPECT_EQ(allocator.getDeallocationCount(), 1);
}

TEST(AllocatorTest, MultipleAllocations) {
    MockAllocator allocator;

    void* ptr1 = allocator.allocate(128, 8);
    void* ptr2 = allocator.allocate(256, 16);
    void* ptr3 = allocator.allocate(512, 32);

    EXPECT_EQ(allocator.getAllocatedSize(), 128 + 256 + 512);
    EXPECT_EQ(allocator.getAllocationCount(), 3);

    allocator.deallocate(ptr2, 256);
    EXPECT_EQ(allocator.getAllocatedSize(), 128 + 512);

    allocator.deallocate(ptr1, 128);
    allocator.deallocate(ptr3, 512);
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

TEST(AllocatorTest, DeallocateNullptr) {
    MockAllocator allocator;
    allocator.deallocate(nullptr, 100);  // Should not crash
    EXPECT_EQ(allocator.getDeallocationCount(), 0);
}

// ============================================================================
// create<T> and destroy<T> tests
// ============================================================================

TEST(AllocatorTest, CreateDestroyPOD) {
    MockAllocator allocator;

    PODType* obj = allocator.create<PODType>();
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(allocator.getAllocationCount(), 1);

    obj->x = 42;
    obj->y = 3.14f;
    obj->z = 2.71828;

    EXPECT_EQ(obj->x, 42);
    EXPECT_FLOAT_EQ(obj->y, 3.14f);
    EXPECT_DOUBLE_EQ(obj->z, 2.71828);

    allocator.destroy(obj);
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

TEST(AllocatorTest, CreateDestroyNonPOD) {
    NonPODType::resetCounters();
    MockAllocator allocator;

    NonPODType* obj = allocator.create<NonPODType>();
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(NonPODType::constructorCalls, 1);
    EXPECT_EQ(obj->getValue(), 0);

    allocator.destroy(obj);
    EXPECT_EQ(NonPODType::destructorCalls, 1);
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

TEST(AllocatorTest, CreateWithArguments) {
    NonPODType::resetCounters();
    MockAllocator allocator;

    NonPODType* obj1 = allocator.create<NonPODType>(42);
    ASSERT_NE(obj1, nullptr);
    EXPECT_EQ(obj1->getValue(), 42);

    NonPODType* obj2 = allocator.create<NonPODType>(10, 20);
    ASSERT_NE(obj2, nullptr);
    EXPECT_EQ(obj2->getValue(), 30);

    EXPECT_EQ(NonPODType::constructorCalls, 2);

    allocator.destroy(obj1);
    allocator.destroy(obj2);

    EXPECT_EQ(NonPODType::destructorCalls, 2);
}

TEST(AllocatorTest, DestroyNullptr) {
    NonPODType::resetCounters();
    MockAllocator allocator;

    allocator.destroy<NonPODType>(nullptr);  // Should not crash
    EXPECT_EQ(NonPODType::destructorCalls, 0);
}

TEST(AllocatorTest, CreateWithAlignment) {
    MockAllocator allocator;

    AlignedType* obj = allocator.create<AlignedType>();
    ASSERT_NE(obj, nullptr);

    // Check alignment
    EXPECT_EQ(reinterpret_cast<uintptr_t>(obj) % 32, 0);

    // Verify we can use it
    for (int i = 0; i < 8; ++i) {
        obj->data[i] = static_cast<float>(i);
    }

    allocator.destroy(obj);
}

// ============================================================================
// allocateArray<T> and deallocateArray<T> tests
// ============================================================================

TEST(AllocatorTest, AllocateArrayPOD) {
    MockAllocator allocator;

    const size_t count = 100;
    float* array = allocator.allocateArray<float>(count);
    ASSERT_NE(array, nullptr);
    EXPECT_EQ(allocator.getAllocatedSize(), sizeof(float) * count);

    // Write and verify
    for (size_t i = 0; i < count; ++i) {
        array[i] = static_cast<float>(i);
    }

    for (size_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(array[i], static_cast<float>(i));
    }

    allocator.deallocateArray(array, count);
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

TEST(AllocatorTest, AllocateArrayZeroCount) {
    MockAllocator allocator;

    float* array = allocator.allocateArray<float>(0);
    EXPECT_EQ(array, nullptr);
}

TEST(AllocatorTest, DeallocateArrayNullptr) {
    MockAllocator allocator;
    allocator.deallocateArray<float>(nullptr, 100);  // Should not crash
}

TEST(AllocatorTest, DeallocateArrayZeroCount) {
    MockAllocator allocator;

    float* array = allocator.allocateArray<float>(10);
    allocator.deallocateArray(array, 0);  // Should not crash, treated as no-op
    // Note: This leaves a leak in the test, but tests the safety of the call
}

// ============================================================================
// allocateArrayWithInit and destroyArray tests
// ============================================================================

TEST(AllocatorTest, AllocateArrayWithInit) {
    NonPODType::resetCounters();
    MockAllocator allocator;

    const size_t count = 10;
    NonPODType* array = allocator.allocateArrayWithInit<NonPODType>(count);
    ASSERT_NE(array, nullptr);

    EXPECT_EQ(NonPODType::constructorCalls, static_cast<int>(count));

    // Verify each element
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(array[i].getValue(), 0);  // Default constructed
    }

    allocator.destroyArray(array, count);
    EXPECT_EQ(NonPODType::destructorCalls, static_cast<int>(count));
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

TEST(AllocatorTest, DestroyArrayPOD) {
    MockAllocator allocator;

    const size_t count = 20;
    float* array = allocator.allocateArray<float>(count);

    allocator.destroyArray(array, count);  // Should work for POD types
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
}

TEST(AllocatorTest, DestroyArrayNullptr) {
    NonPODType::resetCounters();
    MockAllocator allocator;

    allocator.destroyArray<NonPODType>(nullptr, 10);  // Should not crash
    EXPECT_EQ(NonPODType::destructorCalls, 0);
}

// ============================================================================
// Default allocator tests
// ============================================================================

TEST(DefaultAllocatorTest, GetDefaultAllocator) {
    Allocator* allocator = getDefaultAllocator();
    ASSERT_NE(allocator, nullptr);

    // Should be able to use it
    void* ptr = allocator->allocate(1024, 16);
    ASSERT_NE(ptr, nullptr);
    allocator->deallocate(ptr, 1024);
}

TEST(DefaultAllocatorTest, AllocateWithDefault) {
    Allocator* allocator = getDefaultAllocator();

    int* value = allocator->create<int>(42);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 42);

    allocator->destroy(value);
}

TEST(DefaultAllocatorTest, SetDefaultAllocator) {
    MockAllocator customAllocator;

    // Save original
    Allocator* original = getDefaultAllocator();
    ASSERT_NE(original, nullptr);

    // Set custom
    Allocator* previous = setDefaultAllocator(&customAllocator);
    EXPECT_EQ(previous, original);
    EXPECT_EQ(getDefaultAllocator(), &customAllocator);

    // Use custom allocator
    int* value = getDefaultAllocator()->create<int>(123);
    EXPECT_EQ(customAllocator.getAllocationCount(), 1);

    getDefaultAllocator()->destroy(value);
    EXPECT_EQ(customAllocator.getDeallocationCount(), 1);

    // Restore original
    setDefaultAllocator(original);
    EXPECT_EQ(getDefaultAllocator(), original);
}

TEST(DefaultAllocatorTest, SetNullAllocator) {
    Allocator* original = getDefaultAllocator();

    Allocator* result = setDefaultAllocator(nullptr);
    EXPECT_EQ(result, nullptr);

    // Default should remain unchanged
    EXPECT_EQ(getDefaultAllocator(), original);
}

// ============================================================================
// Alignment tests
// ============================================================================

TEST(AllocatorTest, VariousAlignments) {
    MockAllocator allocator;

    const size_t alignments[] = {8, 16, 32, 64};

    for (size_t align : alignments) {
        void* ptr = allocator.allocate(256, align);
        ASSERT_NE(ptr, nullptr);

        uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
        EXPECT_EQ(address % align, 0) << "Failed for alignment " << align;

        allocator.deallocate(ptr, 256);
    }
}

// ============================================================================
// Memory leak detection test
// ============================================================================

TEST(AllocatorTest, NoLeaksAfterCreateDestroy) {
    MockAllocator allocator;

    // Allocate and deallocate in a loop
    for (int i = 0; i < 100; ++i) {
        int* value = allocator.create<int>(i);
        ASSERT_NE(value, nullptr);
        allocator.destroy(value);
    }

    // All memory should be freed
    EXPECT_EQ(allocator.getAllocatedSize(), 0);
    EXPECT_EQ(allocator.getAllocationCount(), allocator.getDeallocationCount());
}

TEST(AllocatorTest, NoLeaksAfterArrayOperations) {
    NonPODType::resetCounters();
    MockAllocator allocator;

    const size_t count = 50;
    for (int iteration = 0; iteration < 10; ++iteration) {
        NonPODType* array = allocator.allocateArrayWithInit<NonPODType>(count);
        ASSERT_NE(array, nullptr);
        allocator.destroyArray(array, count);
    }

    // All memory should be freed
    EXPECT_EQ(allocator.getAllocatedSize(), 0);

    // All objects should be constructed and destructed
    EXPECT_EQ(NonPODType::constructorCalls, NonPODType::destructorCalls);
}
