#include "axiom/memory/stl_adapter.hpp"

#include <gtest/gtest.h>

#include "axiom/memory/heap_allocator.hpp"
#include "axiom/memory/linear_allocator.hpp"
#include "axiom/memory/pool_allocator.hpp"
#include "axiom/memory/stack_allocator.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace axiom::memory {
namespace {

// ============================================================================
// Basic adapter tests
// ============================================================================

TEST(StlAllocatorAdapterTest, DefaultConstruction) {
    StlAllocatorAdapter<int> adapter;
    EXPECT_NE(adapter.getAllocator(), nullptr);
    EXPECT_EQ(adapter.getAllocator(), getDefaultAllocator());
}

TEST(StlAllocatorAdapterTest, CustomAllocatorConstruction) {
    HeapAllocator heapAlloc;
    StlAllocatorAdapter<int> adapter(&heapAlloc);
    EXPECT_EQ(adapter.getAllocator(), &heapAlloc);
}

TEST(StlAllocatorAdapterTest, CopyConstruction) {
    HeapAllocator heapAlloc;
    StlAllocatorAdapter<int> adapter1(&heapAlloc);
    StlAllocatorAdapter<int> adapter2(adapter1);

    EXPECT_EQ(adapter1.getAllocator(), adapter2.getAllocator());
    EXPECT_EQ(adapter2.getAllocator(), &heapAlloc);
}

TEST(StlAllocatorAdapterTest, RebindConstruction) {
    HeapAllocator heapAlloc;
    StlAllocatorAdapter<int> adapter1(&heapAlloc);
    StlAllocatorAdapter<double> adapter2(adapter1);

    EXPECT_EQ(adapter1.getAllocator(), adapter2.getAllocator());
    EXPECT_EQ(adapter2.getAllocator(), &heapAlloc);
}

TEST(StlAllocatorAdapterTest, EqualityComparison) {
    HeapAllocator heapAlloc1;
    HeapAllocator heapAlloc2;

    StlAllocatorAdapter<int> adapter1(&heapAlloc1);
    StlAllocatorAdapter<int> adapter2(&heapAlloc1);
    StlAllocatorAdapter<int> adapter3(&heapAlloc2);

    EXPECT_TRUE(adapter1 == adapter2);
    EXPECT_FALSE(adapter1 == adapter3);
    EXPECT_FALSE(adapter1 != adapter2);
    EXPECT_TRUE(adapter1 != adapter3);
}

TEST(StlAllocatorAdapterTest, RebindEqualityComparison) {
    HeapAllocator heapAlloc;

    StlAllocatorAdapter<int> adapter1(&heapAlloc);
    StlAllocatorAdapter<double> adapter2(&heapAlloc);

    EXPECT_TRUE(adapter1 == adapter2);
    EXPECT_FALSE(adapter1 != adapter2);
}

// ============================================================================
// Allocation and deallocation tests
// ============================================================================

TEST(StlAllocatorAdapterTest, AllocateDeallocate) {
    HeapAllocator heapAlloc;
    StlAllocatorAdapter<int> adapter(&heapAlloc);

    const size_t initialSize = heapAlloc.getAllocatedSize();

    // Allocate
    int* ptr = adapter.allocate(10);
    ASSERT_NE(ptr, nullptr);
    EXPECT_GT(heapAlloc.getAllocatedSize(), initialSize);

    // Write to verify memory is usable
    for (size_t i = 0; i < 10; ++i) {
        ptr[i] = static_cast<int>(i * 2);
    }

    // Verify values
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(ptr[i], static_cast<int>(i * 2));
    }

    // Deallocate
    adapter.deallocate(ptr, 10);
    EXPECT_EQ(heapAlloc.getAllocatedSize(), initialSize);
}

TEST(StlAllocatorAdapterTest, AllocateZeroSize) {
    HeapAllocator heapAlloc;
    StlAllocatorAdapter<int> adapter(&heapAlloc);

    // Allocating zero elements - behavior is implementation defined
    // Just verify it doesn't crash
    bool noException = true;
    int* ptr = nullptr;
    try {
        ptr = adapter.allocate(0);
    } catch (const std::bad_alloc&) {
        noException = false;
    }

    // If allocation succeeded, deallocate safely
    if (noException && ptr) {
        adapter.deallocate(ptr, 0);
    }

    // Test passes as long as no crash occurs
    SUCCEED();
}

TEST(StlAllocatorAdapterTest, AllocateOverflow) {
    HeapAllocator heapAlloc;
    StlAllocatorAdapter<int> adapter(&heapAlloc);

    // Try to allocate an amount that would overflow size_t
    const size_t hugeSize = std::numeric_limits<size_t>::max() / sizeof(int) + 1;

    EXPECT_THROW({
        [[maybe_unused]] int* ptr = adapter.allocate(hugeSize);
    }, std::bad_array_new_length);
}

TEST(StlAllocatorAdapterTest, DeallocateNullptr) {
    HeapAllocator heapAlloc;
    StlAllocatorAdapter<int> adapter(&heapAlloc);

    // Deallocating nullptr should be safe
    EXPECT_NO_THROW(adapter.deallocate(nullptr, 10));
}

// ============================================================================
// std::vector tests
// ============================================================================

TEST(StlAllocatorAdapterTest, VectorBasicOperations) {
    HeapAllocator heapAlloc;
    std::vector<int, StlAllocatorAdapter<int>> vec{StlAllocatorAdapter<int>(&heapAlloc)};

    bool isVecEmpty = vec.empty();
    EXPECT_TRUE(isVecEmpty);
    size_t initialSize = vec.size();
    EXPECT_EQ(initialSize, 0ULL);

    // Push elements
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    size_t vecSize = vec.size();
    EXPECT_EQ(vecSize, 3ULL);
    int val0 = vec.at(0);
    EXPECT_EQ(val0, 1);
    int val1 = vec.at(1);
    EXPECT_EQ(val1, 2);
    int val2 = vec.at(2);
    EXPECT_EQ(val2, 3);

    // Memory should be allocated
    EXPECT_GT(heapAlloc.getAllocatedSize(), size_t{0});

    // Clear and verify memory is freed
    vec.clear();
    vec.shrink_to_fit();
    // Note: std::vector may keep small buffer allocated on some implementations
    EXPECT_LE(heapAlloc.getAllocatedSize(), 16ULL);
}

TEST(StlAllocatorAdapterTest, VectorResize) {
    HeapAllocator heapAlloc;
    std::vector<int, StlAllocatorAdapter<int>> vec{StlAllocatorAdapter<int>(&heapAlloc)};

    vec.resize(100, 42);
    size_t vecSize = vec.size();
    EXPECT_EQ(vecSize, 100ULL);
    EXPECT_GT(heapAlloc.getAllocatedSize(), size_t{0});

    for (size_t idx = 0; idx < vec.size(); ++idx) {
        int val = vec.at(idx);
        EXPECT_EQ(val, 42);
    }
}

TEST(StlAllocatorAdapterTest, VectorWithLinearAllocator) {
    LinearAllocator linearAlloc(1024 * 1024);  // 1MB
    std::vector<float, StlAllocatorAdapter<float>> vec{
        StlAllocatorAdapter<float>(&linearAlloc)};

    const size_t initialUsage = linearAlloc.getAllocatedSize();

    // Allocate some elements
    for (int i = 0; i < 100; ++i) {
        vec.push_back(static_cast<float>(i) * 3.14f);
    }

    size_t vecSize = vec.size();
    EXPECT_EQ(vecSize, 100ULL);
    EXPECT_GT(linearAlloc.getAllocatedSize(), initialUsage);

    // Verify values
    for (size_t i = 0; i < vec.size(); ++i) {
        float actual = vec.at(i);
        float expected = static_cast<float>(i) * 3.14f;
        EXPECT_FLOAT_EQ(actual, expected);
    }

    // Reset linear allocator (bulk deallocation)
    vec.clear();
    vec.shrink_to_fit();
    linearAlloc.reset();
    EXPECT_EQ(linearAlloc.getAllocatedSize(), 0);
}

// ============================================================================
// std::map tests
// ============================================================================

TEST(StlAllocatorAdapterTest, MapBasicOperations) {
    HeapAllocator heapAlloc;
    std::map<int, std::string, std::less<int>, StlAllocatorAdapter<std::pair<const int, std::string>>>
        intStrMap{StlAllocatorAdapter<std::pair<const int, std::string>>(&heapAlloc)};

    bool isEmpty = intStrMap.empty();
    EXPECT_TRUE(isEmpty);

    // Insert elements
    intStrMap.insert({1, "one"});
    intStrMap.insert({2, "two"});
    intStrMap.insert({3, "three"});

    size_t mapSize1 = intStrMap.size();
    EXPECT_EQ(mapSize1, 3ULL);
    std::string val1 = intStrMap.at(1);
    EXPECT_EQ(val1, "one");
    std::string val2 = intStrMap.at(2);
    EXPECT_EQ(val2, "two");
    std::string val3 = intStrMap.at(3);
    EXPECT_EQ(val3, "three");

    // Memory should be allocated
    EXPECT_GT(heapAlloc.getAllocatedSize(), size_t{0});

    // Erase and verify
    intStrMap.erase(2);
    size_t mapSize = intStrMap.size();
    EXPECT_EQ(mapSize, 2ULL);
    size_t count2 = intStrMap.count(2);
    EXPECT_EQ(count2, 0ULL);

    // Clear
    intStrMap.clear();
    // Note: std::map may keep internal tree structure allocated
    EXPECT_LE(heapAlloc.getAllocatedSize(), 96ULL);  // Small residual is acceptable
}

TEST(StlAllocatorAdapterTest, MapRebind) {
    HeapAllocator heapAlloc;
    const size_t initialAllocations = heapAlloc.getAllocationCount();

    {
        std::map<int, int, std::less<int>, StlAllocatorAdapter<std::pair<const int, int>>> map{
            StlAllocatorAdapter<std::pair<const int, int>>(&heapAlloc)};

        // Insert elements (map will allocate nodes internally using rebind)
        for (int i = 0; i < 10; ++i) {
            map.insert({i, i * 2});
        }

        size_t mapSize = map.size();
        EXPECT_EQ(mapSize, 10ULL);
        // Verify that allocations happened (rebind worked)
        EXPECT_GT(heapAlloc.getAllocationCount(), initialAllocations);
    }

    // After map destruction, all memory should be freed
    EXPECT_EQ(heapAlloc.getAllocatedSize(), 0ULL);
}

// ============================================================================
// std::set tests
// ============================================================================

TEST(StlAllocatorAdapterTest, SetBasicOperations) {
    HeapAllocator heapAlloc;
    std::set<int, std::less<int>, StlAllocatorAdapter<int>> intSet{
        StlAllocatorAdapter<int>(&heapAlloc)};

    bool isEmpty = intSet.empty();
    EXPECT_TRUE(isEmpty);

    // Insert elements
    intSet.insert(3);
    intSet.insert(1);
    intSet.insert(2);

    size_t setSize = intSet.size();
    EXPECT_EQ(setSize, 3ULL);
    size_t count1 = intSet.count(1);
    EXPECT_EQ(count1, 1ULL);
    size_t count2 = intSet.count(2);
    EXPECT_EQ(count2, 1ULL);
    size_t count3 = intSet.count(3);
    EXPECT_EQ(count3, 1ULL);

    // Verify ordering
    std::vector<int> values(intSet.begin(), intSet.end());
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 3);

    // Clear
    intSet.clear();
    // Note: std::set may keep internal tree structure allocated
    EXPECT_LE(heapAlloc.getAllocatedSize(), 64ULL);  // Small residual is acceptable
}

// ============================================================================
// std::unordered_map tests
// ============================================================================

TEST(StlAllocatorAdapterTest, UnorderedMapBasicOperations) {
    HeapAllocator heapAlloc;
    std::unordered_map<int, std::string, std::hash<int>, std::equal_to<int>,
                       StlAllocatorAdapter<std::pair<const int, std::string>>>
        unorderedMap{StlAllocatorAdapter<std::pair<const int, std::string>>(&heapAlloc)};

    bool isEmpty = unorderedMap.empty();
    EXPECT_TRUE(isEmpty);

    // Insert elements
    unorderedMap.insert({1, "one"});
    unorderedMap.insert({2, "two"});
    unorderedMap.insert({3, "three"});

    size_t mapSize = unorderedMap.size();
    EXPECT_EQ(mapSize, 3ULL);
    std::string val1 = unorderedMap.at(1);
    EXPECT_EQ(val1, "one");
    std::string val2 = unorderedMap.at(2);
    EXPECT_EQ(val2, "two");
    std::string val3 = unorderedMap.at(3);
    EXPECT_EQ(val3, "three");

    // Clear
    unorderedMap.clear();
    // Note: std::unordered_map keeps bucket array allocated after clear()
    size_t finalSize = heapAlloc.getAllocatedSize();
    EXPECT_LE(finalSize, 300ULL);  // Bucket array remains
}

// ============================================================================
// std::unordered_set tests
// ============================================================================

TEST(StlAllocatorAdapterTest, UnorderedSetBasicOperations) {
    HeapAllocator heapAlloc;
    std::unordered_set<int, std::hash<int>, std::equal_to<int>, StlAllocatorAdapter<int>> intSet{
        StlAllocatorAdapter<int>(&heapAlloc)};

    bool isEmpty = intSet.empty();
    EXPECT_TRUE(isEmpty);

    // Insert elements
    intSet.insert(1);
    intSet.insert(2);
    intSet.insert(3);

    size_t setSize = intSet.size();
    EXPECT_EQ(setSize, 3ULL);
    size_t count1 = intSet.count(1);
    EXPECT_EQ(count1, 1ULL);
    size_t count2 = intSet.count(2);
    EXPECT_EQ(count2, 1ULL);
    size_t count3 = intSet.count(3);
    EXPECT_EQ(count3, 1ULL);

    // Clear
    intSet.clear();
    // Note: std::unordered_set keeps bucket array allocated after clear()
    EXPECT_LE(heapAlloc.getAllocatedSize(), 200ULL);  // Bucket array remains
}

// ============================================================================
// Type alias tests
// ============================================================================

TEST(StlAllocatorAdapterTest, VectorAlias) {
    Vector<int> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    EXPECT_EQ(vec.size(), 3ULL);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
}

TEST(StlAllocatorAdapterTest, MapAlias) {
    Map<int, std::string> map;

    map.insert({1, "one"});
    map.insert({2, "two"});

    size_t mapSize = map.size();
    EXPECT_EQ(mapSize, 2ULL);
    std::string val1 = map.at(1);
    EXPECT_EQ(val1, "one");
    std::string val2 = map.at(2);
    EXPECT_EQ(val2, "two");
}

TEST(StlAllocatorAdapterTest, SetAlias) {
    Set<int> intSet;

    intSet.insert(1);
    intSet.insert(2);
    intSet.insert(3);

    size_t setSize = intSet.size();
    EXPECT_EQ(setSize, 3ULL);
    size_t count1 = intSet.count(1);
    EXPECT_EQ(count1, 1ULL);
}

TEST(StlAllocatorAdapterTest, UnorderedMapAlias) {
    UnorderedMap<int, std::string> map;

    map.insert({1, "one"});
    map.insert({2, "two"});

    size_t mapSize = map.size();
    EXPECT_EQ(mapSize, 2ULL);
    std::string val1 = map.at(1);
    EXPECT_EQ(val1, "one");
}

TEST(StlAllocatorAdapterTest, UnorderedSetAlias) {
    UnorderedSet<int> intSet;

    intSet.insert(1);
    intSet.insert(2);

    size_t setSize = intSet.size();
    EXPECT_EQ(setSize, 2ULL);
    size_t count1 = intSet.count(1);
    EXPECT_EQ(count1, 1ULL);
}

// ============================================================================
// Custom allocator integration tests
// ============================================================================

TEST(StlAllocatorAdapterTest, LinearAllocatorIntegration) {
    // Note: StackAllocator doesn't work well with std::vector due to LIFO violations
    // during reallocation. Use LinearAllocator instead for dynamic containers.
    LinearAllocator linearAlloc(1024);
    std::vector<int, StlAllocatorAdapter<int>> vec{StlAllocatorAdapter<int>(&linearAlloc)};

    // Allocate elements
    for (int i = 0; i < 10; ++i) {
        vec.push_back(i);
    }

    size_t vecSize = vec.size();
    EXPECT_EQ(vecSize, 10ULL);
    EXPECT_GT(linearAlloc.getAllocatedSize(), 0ULL);

    // Reset linear allocator (bulk deallocation)
    vec.clear();
    vec.shrink_to_fit();
    linearAlloc.reset();
    EXPECT_EQ(linearAlloc.getAllocatedSize(), 0ULL);
}

TEST(StlAllocatorAdapterTest, MultipleContainersSameAllocator) {
    HeapAllocator heapAlloc;

    std::vector<int, StlAllocatorAdapter<int>> vec{StlAllocatorAdapter<int>(&heapAlloc)};
    std::map<int, int, std::less<int>, StlAllocatorAdapter<std::pair<const int, int>>> intMap{
        StlAllocatorAdapter<std::pair<const int, int>>(&heapAlloc)};

    // Use both containers
    vec.push_back(1);
    vec.push_back(2);
    intMap.insert({1, 10});
    intMap.insert({2, 20});

    // Both should use the same allocator
    EXPECT_GT(heapAlloc.getAllocatedSize(), 0ULL);

    // Clear both
    vec.clear();
    vec.shrink_to_fit();
    intMap.clear();

    // Note: std::map may keep internal tree structure allocated
    EXPECT_LE(heapAlloc.getAllocatedSize(), 100ULL);
}

// ============================================================================
// Memory statistics tests
// ============================================================================

TEST(StlAllocatorAdapterTest, MemoryStatisticsTracking) {
    HeapAllocator heapAlloc;
    const size_t initialCount = heapAlloc.getAllocationCount();
    const size_t initialSize = heapAlloc.getAllocatedSize();

    {
        std::vector<int, StlAllocatorAdapter<int>> vec{StlAllocatorAdapter<int>(&heapAlloc)};

        // Reserve capacity
        vec.reserve(100);

        // Should have allocated
        EXPECT_GT(heapAlloc.getAllocationCount(), initialCount);
        EXPECT_GT(heapAlloc.getAllocatedSize(), initialSize);

        // Add elements (may cause reallocation)
        for (int i = 0; i < 200; ++i) {
            vec.push_back(i);
        }

        // Should have reallocated
        EXPECT_GT(heapAlloc.getAllocationCount(), initialCount + 1);
    }

    // After vector destruction, memory should be freed
    EXPECT_EQ(heapAlloc.getAllocatedSize(), initialSize);
}

TEST(StlAllocatorAdapterTest, PeakMemoryTracking) {
    HeapAllocator heapAlloc;

    {
        std::vector<int, StlAllocatorAdapter<int>> vec{StlAllocatorAdapter<int>(&heapAlloc)};

        // Allocate a large amount
        vec.resize(10000, 42);

        const size_t peak = heapAlloc.getPeakAllocatedSize();
        EXPECT_GT(peak, 0);

        // Shrink
        vec.resize(100);
        vec.shrink_to_fit();

        // Peak should remain near the high-water mark (may vary slightly due to reallocation)
        EXPECT_GE(heapAlloc.getPeakAllocatedSize(), peak);
    }
}

// ============================================================================
// Complex type tests
// ============================================================================

struct ComplexType {
    int value;
    std::string name;
    std::vector<double> data;

    ComplexType() : value(0), name("default") {}
    ComplexType(int v, const std::string& n) : value(v), name(n) {}

    bool operator<(const ComplexType& other) const { return value < other.value; }
};

TEST(StlAllocatorAdapterTest, VectorOfComplexTypes) {
    HeapAllocator heapAlloc;
    std::vector<ComplexType, StlAllocatorAdapter<ComplexType>> vec{
        StlAllocatorAdapter<ComplexType>(&heapAlloc)};

    vec.emplace_back(1, "first");
    vec.emplace_back(2, "second");
    vec.emplace_back(3, "third");

    size_t vecSize = vec.size();
    EXPECT_EQ(vecSize, 3ULL);

    ComplexType elem0 = vec.at(0);
    EXPECT_EQ(elem0.value, 1);
    EXPECT_STREQ(elem0.name.c_str(), "first");

    ComplexType elem1 = vec.at(1);
    EXPECT_EQ(elem1.value, 2);
    EXPECT_STREQ(elem1.name.c_str(), "second");

    vec.clear();
    vec.shrink_to_fit();
}

TEST(StlAllocatorAdapterTest, SetOfComplexTypes) {
    HeapAllocator heapAlloc;
    using ComplexSet = std::set<ComplexType, std::less<ComplexType>, StlAllocatorAdapter<ComplexType>>;
    ComplexSet mySet{StlAllocatorAdapter<ComplexType>(&heapAlloc)};

    mySet.emplace(3, "third");
    mySet.emplace(1, "first");
    mySet.emplace(2, "second");

    size_t actualSize = mySet.size();
    EXPECT_EQ(actualSize, 3ULL);

    // Should be sorted by value - collect values in a vector to check ordering
    std::vector<int> values;
    for (ComplexSet::const_iterator it = mySet.begin(); it != mySet.end(); ++it) {
        values.push_back(it->value);
    }

    size_t actualValuesSize = values.size();
    ASSERT_EQ(actualValuesSize, 3ULL);
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 3);

    mySet.clear();
}

}  // namespace
}  // namespace axiom::memory
