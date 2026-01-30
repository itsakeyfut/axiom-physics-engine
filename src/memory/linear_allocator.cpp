#include "axiom/memory/linear_allocator.hpp"

#include <algorithm>
#include <cstring>

namespace axiom::memory {

// ============================================================================
// LinearAllocator implementation
// ============================================================================

LinearAllocator::LinearAllocator(size_t capacity)
    : buffer_(nullptr),
      capacity_(capacity),
      offset_(0),
      peakUsage_(0),
      allocationCount_(0) {
    if (capacity > 0) {
        // Allocate buffer with 64-byte alignment for SIMD
        buffer_ = static_cast<uint8_t*>(alignedAlloc(capacity, 64));
        if (!buffer_) {
            // Allocation failed - set capacity to 0
            capacity_ = 0;
        }
    }
}

LinearAllocator::~LinearAllocator() {
    if (buffer_) {
        alignedFree(buffer_);
        buffer_ = nullptr;
    }
}

void* LinearAllocator::allocate(size_t size, size_t alignment) {
    if (size == 0 || !buffer_) {
        return nullptr;
    }

    // Verify alignment is power of 2
    if ((alignment & (alignment - 1)) != 0) {
        return nullptr;
    }

    // Align current offset to required alignment
    const size_t alignedOffset = alignUp(offset_, alignment);

    // Check if we have enough space
    const size_t newOffset = alignedOffset + size;
    if (newOffset > capacity_) {
        return nullptr;  // Out of memory
    }

    // Allocate by bumping pointer
    void* ptr = buffer_ + alignedOffset;
    offset_ = newOffset;

    // Update statistics
    ++allocationCount_;
    updatePeak();

    return ptr;
}

void LinearAllocator::deallocate(void* ptr, size_t size) {
    // No-op: linear allocators don't support individual deallocation
    (void)ptr;
    (void)size;
}

size_t LinearAllocator::getAllocatedSize() const {
    return offset_;
}

size_t LinearAllocator::capacity() const {
    return capacity_;
}

size_t LinearAllocator::remaining() const {
    return capacity_ - offset_;
}

size_t LinearAllocator::getPeakUsage() const {
    return peakUsage_;
}

size_t LinearAllocator::getAllocationCount() const {
    return allocationCount_;
}

void LinearAllocator::reset() {
    offset_ = 0;
}

LinearAllocator::Marker LinearAllocator::getMarker() const {
    return offset_;
}

void LinearAllocator::resetToMarker(Marker marker) {
    // Only reset if marker is valid (less than or equal to current offset)
    if (marker <= offset_) {
        offset_ = marker;
    }
}

bool LinearAllocator::owns(void* ptr) const {
    if (!ptr || !buffer_) {
        return false;
    }

    const uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);
    const uintptr_t bufferStart = reinterpret_cast<uintptr_t>(buffer_);
    const uintptr_t bufferEnd = bufferStart + capacity_;

    return ptrAddr >= bufferStart && ptrAddr < bufferEnd;
}

void LinearAllocator::resetStatistics() {
    peakUsage_ = offset_;
    allocationCount_ = 0;
}

size_t LinearAllocator::alignUp(size_t value, size_t alignment) {
    // alignment must be power of 2
    return (value + alignment - 1) & ~(alignment - 1);
}

void LinearAllocator::updatePeak() {
    if (offset_ > peakUsage_) {
        peakUsage_ = offset_;
    }
}

// ============================================================================
// LinearAllocatorScope implementation
// ============================================================================

LinearAllocatorScope::LinearAllocatorScope(LinearAllocator& allocator)
    : allocator_(allocator), marker_(allocator.getMarker()) {}

LinearAllocatorScope::~LinearAllocatorScope() {
    allocator_.resetToMarker(marker_);
}

// ============================================================================
// FrameAllocator implementation
// ============================================================================

FrameAllocator::FrameAllocator(size_t totalCapacity)
    : buffers_{nullptr, nullptr}, currentBuffer_(0), frameNumber_(0) {
    // Split capacity between two buffers
    const size_t bufferCapacity = totalCapacity / 2;

    // Allocate both buffers
    buffers_[0] = new LinearAllocator(bufferCapacity);
    buffers_[1] = new LinearAllocator(bufferCapacity);
}

FrameAllocator::~FrameAllocator() {
    delete buffers_[0];
    delete buffers_[1];
}

void* FrameAllocator::allocate(size_t size, size_t alignment) {
    return buffers_[currentBuffer_]->allocate(size, alignment);
}

void FrameAllocator::deallocate(void* ptr, size_t size) {
    // No-op: frame allocators don't support individual deallocation
    (void)ptr;
    (void)size;
}

size_t FrameAllocator::getAllocatedSize() const {
    return buffers_[0]->getAllocatedSize() + buffers_[1]->getAllocatedSize();
}

void FrameAllocator::flip() {
    // Switch to next buffer
    currentBuffer_ = 1 - currentBuffer_;

    // Reset the buffer we just switched to (it contains data from previous frame)
    buffers_[currentBuffer_]->reset();

    // Increment frame counter
    ++frameNumber_;
}

size_t FrameAllocator::getFrameNumber() const {
    return frameNumber_;
}

size_t FrameAllocator::getBufferCapacity() const {
    return buffers_[0]->capacity();
}

size_t FrameAllocator::remaining() const {
    return buffers_[currentBuffer_]->remaining();
}

size_t FrameAllocator::getPeakUsage() const {
    return std::max(buffers_[0]->getPeakUsage(), buffers_[1]->getPeakUsage());
}

}  // namespace axiom::memory
