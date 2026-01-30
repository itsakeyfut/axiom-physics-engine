#include "axiom/memory/stack_allocator.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

namespace axiom::memory {

// ============================================================================
// StackAllocator implementation
// ============================================================================

StackAllocator::StackAllocator(size_t capacity)
    : buffer_(nullptr),
      capacity_(capacity),
      offset_(0),
      peakUsage_(0),
      allocationCount_(0),
      deallocationCount_(0) {
    if (capacity > 0) {
        // Allocate buffer with 64-byte alignment for SIMD
        buffer_ = static_cast<uint8_t*>(alignedAlloc(capacity, 64));
        if (!buffer_) {
            // Allocation failed - set capacity to 0
            capacity_ = 0;
        }
    }
}

StackAllocator::~StackAllocator() {
#ifndef NDEBUG
    // In debug builds, warn if there are still active allocations
    if (getActiveAllocationCount() > 0) {
        // Note: In production code, you might want to use a proper logging system
        // For now, we just silently clean up
    }
#endif

    if (buffer_) {
        alignedFree(buffer_);
        buffer_ = nullptr;
    }
}

void* StackAllocator::allocate(size_t size, size_t alignment) {
    if (size == 0 || !buffer_) {
        return nullptr;
    }

    // Verify alignment is power of 2
    if ((alignment & (alignment - 1)) != 0) {
        return nullptr;
    }

    // Memory layout: [header][padding][header_ptr][data]
    // The header_ptr points back to the header, allowing us to find it reliably

    // Start with current offset aligned for header
    const size_t headerOffset = alignUp(offset_, alignof(AllocationHeader));

    // Calculate where we'd start placing data+pointer after header
    const size_t afterHeader = headerOffset + sizeof(AllocationHeader);

    // We need to place: [header_ptr (8 bytes)][data (aligned to 'alignment')]
    // So we need to find an offset where:
    // - There's room for a pointer (8 bytes)
    // - The data after the pointer is aligned to 'alignment'

    // Calculate where the data should be aligned
    // We need: dataOffset % alignment == 0
    // And: headerPtrOffset = dataOffset - sizeof(AllocationHeader*)

    // Start from afterHeader and find the next position where we can fit both
    size_t dataOffset = alignUp(afterHeader + sizeof(AllocationHeader*), alignment);
    size_t headerPtrOffset = dataOffset - sizeof(AllocationHeader*);

    // Check if we have enough space
    const size_t newOffset = dataOffset + size;
    if (newOffset > capacity_) {
        return nullptr;  // Out of memory
    }

    // Write allocation header
    AllocationHeader* header = reinterpret_cast<AllocationHeader*>(buffer_ + headerOffset);
    header->prevOffset = offset_;
    header->size = size;

    // Store pointer to header immediately before the data
    AllocationHeader** headerPtr = reinterpret_cast<AllocationHeader**>(buffer_ + headerPtrOffset);
    *headerPtr = header;

    // Allocate by bumping pointer
    void* ptr = buffer_ + dataOffset;
    offset_ = newOffset;

    // Update statistics
    ++allocationCount_;
    updatePeak();

    return ptr;
}

void StackAllocator::deallocate(void* ptr, size_t size) {
    if (!ptr || !buffer_) {
        return;  // nullptr is safe to deallocate
    }

    // Verify that ptr is within our buffer
    if (!owns(ptr)) {
        assert(false && "Pointer not allocated from this StackAllocator");
        return;
    }

    // Get the header
    AllocationHeader* header = getHeader(ptr);

    // In debug builds, verify LIFO order and size
#ifndef NDEBUG
    // Verify size matches
    if (header->size != size) {
        assert(false && "Deallocate size mismatch");
        return;
    }

    // Verify LIFO order: ptr should be at the end of our current allocations
    const uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);
    const uintptr_t expectedEnd = reinterpret_cast<uintptr_t>(buffer_) + offset_;

    // The allocation should end at or near the current offset
    // (allowing for alignment padding after the allocation)
    if (ptrAddr + size > expectedEnd) {
        assert(false && "LIFO violation: attempting to deallocate non-top allocation");
        return;
    }
#else
    (void)size;  // Unused in release builds
#endif

    // Restore offset to previous allocation
    offset_ = header->prevOffset;

    // Update statistics
    ++deallocationCount_;
}

size_t StackAllocator::getAllocatedSize() const {
    return offset_;
}

size_t StackAllocator::capacity() const {
    return capacity_;
}

size_t StackAllocator::remaining() const {
    return capacity_ - offset_;
}

size_t StackAllocator::getPeakUsage() const {
    return peakUsage_;
}

size_t StackAllocator::getAllocationCount() const {
    return allocationCount_;
}

size_t StackAllocator::getDeallocationCount() const {
    return deallocationCount_;
}

size_t StackAllocator::getActiveAllocationCount() const {
    // Active allocations = allocations - deallocations
    // (assumes LIFO order is followed)
    return allocationCount_ - deallocationCount_;
}

void StackAllocator::reset() {
#ifndef NDEBUG
    // In debug builds, warn if there are still active allocations
    if (getActiveAllocationCount() > 0) {
        // Resetting with active allocations may indicate a memory management issue
        // In production, you might want to log this
    }
#endif

    offset_ = 0;
}

bool StackAllocator::owns(void* ptr) const {
    if (!ptr || !buffer_) {
        return false;
    }

    const uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);
    const uintptr_t bufferStart = reinterpret_cast<uintptr_t>(buffer_);
    const uintptr_t bufferEnd = bufferStart + capacity_;

    return ptrAddr >= bufferStart && ptrAddr < bufferEnd;
}

void StackAllocator::resetStatistics() {
    peakUsage_ = offset_;
    allocationCount_ = 0;
    deallocationCount_ = 0;
}

size_t StackAllocator::alignUp(size_t value, size_t alignment) {
    // alignment must be power of 2
    return (value + alignment - 1) & ~(alignment - 1);
}

void StackAllocator::updatePeak() {
    if (offset_ > peakUsage_) {
        peakUsage_ = offset_;
    }
}

StackAllocator::AllocationHeader* StackAllocator::getHeader(void* ptr) const {
    // The header pointer is stored immediately before the data pointer
    AllocationHeader** headerPtr = reinterpret_cast<AllocationHeader**>(static_cast<uint8_t*>(ptr) -
                                                                        sizeof(AllocationHeader*));
    return *headerPtr;
}

}  // namespace axiom::memory
