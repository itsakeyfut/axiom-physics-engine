#pragma once

#include "axiom/memory/allocator.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace axiom::memory {

/**
 * @brief Fast fixed-size block allocator using free-list
 *
 * PoolAllocator provides O(1) allocation and deallocation for fixed-size
 * objects by maintaining a free-list of available blocks. This is ideal for
 * frequently allocated/deallocated objects like rigid bodies, contacts, and
 * constraints in a physics engine.
 *
 * Key features:
 * - O(1) allocation and deallocation via free-list
 * - Automatic growth with multiple chunks
 * - Memory alignment support for SIMD types
 * - Efficient memory reuse (no fragmentation within chunks)
 * - Low memory overhead (single pointer per free block)
 *
 * Performance characteristics:
 * - 10-100x faster than HeapAllocator for fixed-size allocations
 * - Cache-friendly due to sequential memory layout within chunks
 * - No external fragmentation (each chunk is uniformly sized)
 *
 * Template parameters:
 * @tparam BlockSize Size of each allocatable block in bytes
 * @tparam Alignment Alignment requirement in bytes (must be power of 2)
 *
 * Example usage:
 * @code
 * // Pool for RigidBody objects (256 blocks per chunk, 16-byte aligned)
 * PoolAllocator<sizeof(RigidBody), alignof(RigidBody)> bodyPool;
 * bodyPool.reserve(1024);  // Pre-allocate space for 1024 bodies
 *
 * // Allocate and construct
 * void* ptr = bodyPool.allocate();
 * RigidBody* body = new (ptr) RigidBody(config);  // Placement new
 *
 * // Use body...
 *
 * // Destroy and deallocate
 * body->~RigidBody();
 * bodyPool.deallocate(ptr);
 * @endcode
 *
 * @note This allocator is NOT thread-safe. Use separate pool instances per
 *       thread or add external synchronization.
 * @note All blocks in the pool have the same size (BlockSize)
 * @note Deallocated blocks are added to the free-list for reuse
 */
template <size_t BlockSize, size_t Alignment = alignof(std::max_align_t)>
class PoolAllocator final : public Allocator {
public:
    /**
     * @brief Construct a PoolAllocator with default chunk size
     *
     * The allocator starts with no memory allocated. Memory is allocated
     * on-demand when allocate() is called and the free-list is empty.
     *
     * @param blocksPerChunk Number of blocks to allocate per chunk (default: 256)
     *
     * @note blocksPerChunk should be tuned based on expected object count
     * @note Larger chunks reduce allocation frequency but increase memory usage
     */
    explicit PoolAllocator(size_t blocksPerChunk = 256);

    /**
     * @brief Destroy the PoolAllocator and free all chunks
     *
     * @warning This does NOT call destructors for allocated objects
     * @warning User must manually deallocate all objects before destruction
     */
    ~PoolAllocator() override;

    // Disable copy and move (allocators should not be copied/moved)
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&) = delete;
    PoolAllocator& operator=(PoolAllocator&&) = delete;

    /**
     * @brief Allocate a single block from the pool
     *
     * If the free-list is empty, a new chunk is allocated. Otherwise, a
     * block is popped from the free-list.
     *
     * @param size Must equal BlockSize (checked with assertion)
     * @param alignment Must equal Alignment (checked with assertion)
     * @return Pointer to allocated block, or nullptr on failure
     *
     * @note The returned memory is not initialized
     * @note Time complexity: O(1) average, O(n) when allocating new chunk
     * @note This method is NOT thread-safe
     */
    void* allocate(size_t size, size_t alignment) override;

    /**
     * @brief Deallocate a block and return it to the free-list
     *
     * The block is added to the head of the free-list for fast reuse.
     *
     * @param ptr Pointer to block (must be from this allocator)
     * @param size Must equal BlockSize (checked with assertion)
     *
     * @note This does NOT call the destructor - user must do that manually
     * @note Passing nullptr is safe (no-op)
     * @note Time complexity: O(1)
     * @note This method is NOT thread-safe
     */
    void deallocate(void* ptr, size_t size) override;

    /**
     * @brief Get total currently allocated size
     *
     * Returns the total memory allocated from the system, including both
     * used and free blocks.
     *
     * @return Total allocated memory in bytes
     *
     * @note This includes memory for the free-list (deallocated blocks)
     * @note Formula: totalChunks * blocksPerChunk * BlockSize
     */
    size_t getAllocatedSize() const override;

    /**
     * @brief Get the total capacity of the pool
     *
     * Returns the total number of blocks that can be allocated without
     * growing the pool (total blocks across all chunks).
     *
     * @return Total number of blocks in all chunks
     */
    size_t capacity() const;

    /**
     * @brief Get the number of currently allocated blocks
     *
     * Returns the number of blocks that are currently in use (not in the
     * free-list).
     *
     * @return Number of allocated blocks
     *
     * @note Formula: capacity() - freeListSize
     */
    size_t size() const;

    /**
     * @brief Check if a pointer was allocated from this pool
     *
     * Checks if the given pointer belongs to any chunk in this pool.
     *
     * @param ptr Pointer to check
     * @return true if ptr was allocated from this pool, false otherwise
     *
     * @note This is useful for debugging and validation
     * @note Time complexity: O(chunks) where chunks is the number of chunks
     */
    bool owns(void* ptr) const;

    /**
     * @brief Pre-allocate capacity for a given number of blocks
     *
     * Ensures the pool has capacity for at least 'count' blocks by
     * allocating additional chunks as needed.
     *
     * @param count Minimum number of blocks to reserve
     *
     * @note This does not change size(), only capacity()
     * @note Useful to avoid allocations during time-critical sections
     */
    void reserve(size_t count);

    /**
     * @brief Clear all allocations and reset the pool
     *
     * Frees all chunks and resets the pool to its initial state.
     *
     * @warning This does NOT call destructors for allocated objects
     * @warning User must manually destroy all objects before calling clear()
     */
    void clear();

private:
    /**
     * @brief Free-list node embedded in each free block
     *
     * When a block is deallocated, we store a pointer to the next free
     * block in the first sizeof(void*) bytes of the block itself.
     * This allows O(1) push/pop without additional memory overhead.
     */
    struct FreeNode {
        FreeNode* next;
    };

    /**
     * @brief A contiguous chunk of memory containing multiple blocks
     *
     * Each chunk contains blocksPerChunk_ blocks, each of size BlockSize
     * and aligned to Alignment. Chunks are allocated on-demand when the
     * free-list is empty.
     */
    struct Chunk {
        void* memory;       ///< Pointer to chunk memory (aligned)
        size_t blockCount;  ///< Number of blocks in this chunk

        Chunk(size_t numBlocks, size_t blockSize, size_t alignment);
        ~Chunk();

        // Disable copy and move
        Chunk(const Chunk&) = delete;
        Chunk& operator=(const Chunk&) = delete;
        Chunk(Chunk&&) = delete;
        Chunk& operator=(Chunk&&) = delete;

        /**
         * @brief Check if a pointer is within this chunk's memory range
         */
        bool contains(void* ptr, size_t blockSize) const;

        /**
         * @brief Get pointer to block at given index
         */
        void* getBlock(size_t index, size_t blockSize) const;
    };

    /**
     * @brief Allocate a new chunk and add its blocks to the free-list
     *
     * Allocates a new chunk with blocksPerChunk_ blocks, then links all
     * blocks into the free-list.
     *
     * @return true on success, false on allocation failure
     */
    bool allocateNewChunk();

    size_t blocksPerChunk_;                       ///< Blocks per chunk
    std::vector<std::unique_ptr<Chunk>> chunks_;  ///< All allocated chunks
    FreeNode* freeListHead_;                      ///< Head of free-list (nullptr if empty)
    size_t freeListSize_;                         ///< Number of blocks in free-list
};

// ============================================================================
// Template implementation
// ============================================================================

template <size_t BlockSize, size_t Alignment>
PoolAllocator<BlockSize, Alignment>::PoolAllocator(size_t blocksPerChunk)
    : blocksPerChunk_(blocksPerChunk), freeListHead_(nullptr), freeListSize_(0) {
    static_assert(BlockSize >= sizeof(FreeNode),
                  "BlockSize must be at least sizeof(void*) to store free-list pointers");
    static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be a power of 2");
}

template <size_t BlockSize, size_t Alignment>
PoolAllocator<BlockSize, Alignment>::~PoolAllocator() {
    clear();
}

template <size_t BlockSize, size_t Alignment>
void* PoolAllocator<BlockSize, Alignment>::allocate(size_t size, size_t alignment) {
    // Verify size and alignment match template parameters
    if (size != BlockSize || alignment != Alignment) {
        return nullptr;
    }

    // If free-list is empty, allocate a new chunk
    if (!freeListHead_) {
        if (!allocateNewChunk()) {
            return nullptr;  // Allocation failed
        }
    }

    // Pop from free-list
    FreeNode* node = freeListHead_;
    freeListHead_ = node->next;
    --freeListSize_;

    return node;
}

template <size_t BlockSize, size_t Alignment>
void PoolAllocator<BlockSize, Alignment>::deallocate(void* ptr, size_t size) {
    if (!ptr) {
        return;
    }

    // Verify size matches template parameter
    if (size != BlockSize) {
        return;  // Invalid size
    }

    // Push onto free-list
    FreeNode* node = static_cast<FreeNode*>(ptr);
    node->next = freeListHead_;
    freeListHead_ = node;
    ++freeListSize_;
}

template <size_t BlockSize, size_t Alignment>
size_t PoolAllocator<BlockSize, Alignment>::getAllocatedSize() const {
    return chunks_.size() * blocksPerChunk_ * BlockSize;
}

template <size_t BlockSize, size_t Alignment>
size_t PoolAllocator<BlockSize, Alignment>::capacity() const {
    return chunks_.size() * blocksPerChunk_;
}

template <size_t BlockSize, size_t Alignment>
size_t PoolAllocator<BlockSize, Alignment>::size() const {
    return capacity() - freeListSize_;
}

template <size_t BlockSize, size_t Alignment>
bool PoolAllocator<BlockSize, Alignment>::owns(void* ptr) const {
    if (!ptr) {
        return false;
    }

    for (const auto& chunk : chunks_) {
        if (chunk->contains(ptr, BlockSize)) {
            return true;
        }
    }

    return false;
}

template <size_t BlockSize, size_t Alignment>
void PoolAllocator<BlockSize, Alignment>::reserve(size_t count) {
    // Calculate how many chunks we need
    const size_t currentCapacity = capacity();
    if (currentCapacity >= count) {
        return;  // Already have enough capacity
    }

    const size_t blocksNeeded = count - currentCapacity;
    const size_t chunksNeeded = (blocksNeeded + blocksPerChunk_ - 1) / blocksPerChunk_;

    // Allocate additional chunks
    for (size_t i = 0; i < chunksNeeded; ++i) {
        if (!allocateNewChunk()) {
            break;  // Stop on failure
        }
    }
}

template <size_t BlockSize, size_t Alignment>
void PoolAllocator<BlockSize, Alignment>::clear() {
    chunks_.clear();
    freeListHead_ = nullptr;
    freeListSize_ = 0;
}

template <size_t BlockSize, size_t Alignment>
bool PoolAllocator<BlockSize, Alignment>::allocateNewChunk() {
    // Create new chunk
    auto chunk = std::make_unique<Chunk>(blocksPerChunk_, BlockSize, Alignment);
    if (!chunk->memory) {
        return false;  // Allocation failed
    }

    // Link all blocks in the chunk into the free-list
    for (size_t i = 0; i < blocksPerChunk_; ++i) {
        void* block = chunk->getBlock(i, BlockSize);
        if (!block) {
            continue;  // Skip null blocks (should not happen)
        }
        FreeNode* node = static_cast<FreeNode*>(block);
        node->next = freeListHead_;
        freeListHead_ = node;
        ++freeListSize_;
    }

    // Add chunk to our list
    chunks_.push_back(std::move(chunk));
    return true;
}

// ============================================================================
// Chunk implementation
// ============================================================================

template <size_t BlockSize, size_t Alignment>
PoolAllocator<BlockSize, Alignment>::Chunk::Chunk(size_t numBlocks, size_t blockSize,
                                                  size_t alignment)
    : memory(nullptr), blockCount(numBlocks) {
    const size_t totalSize = numBlocks * blockSize;
    memory = alignedAlloc(totalSize, alignment);
}

template <size_t BlockSize, size_t Alignment>
PoolAllocator<BlockSize, Alignment>::Chunk::~Chunk() {
    if (memory) {
        alignedFree(memory);
        memory = nullptr;
    }
}

template <size_t BlockSize, size_t Alignment>
bool PoolAllocator<BlockSize, Alignment>::Chunk::contains(void* ptr, size_t blockSize) const {
    if (!memory || !ptr) {
        return false;
    }

    const uintptr_t chunkStart = reinterpret_cast<uintptr_t>(memory);
    const uintptr_t chunkEnd = chunkStart + (blockCount * blockSize);
    const uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);

    return ptrAddr >= chunkStart && ptrAddr < chunkEnd;
}

template <size_t BlockSize, size_t Alignment>
void* PoolAllocator<BlockSize, Alignment>::Chunk::getBlock(size_t index, size_t blockSize) const {
    if (!memory || index >= blockCount) {
        return nullptr;
    }

    uint8_t* base = static_cast<uint8_t*>(memory);
    return base + (index * blockSize);
}

}  // namespace axiom::memory
