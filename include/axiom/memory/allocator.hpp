#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

namespace axiom::memory {

/**
 * @brief Abstract allocator interface for custom memory management
 *
 * The Allocator class provides a unified interface for memory allocation
 * strategies in the Axiom Physics Engine. Different allocators can implement
 * this interface to provide specialized allocation strategies optimized for
 * specific use cases (e.g., heap, pool, linear, stack allocators).
 *
 * All allocators must support aligned allocations to work with SIMD types
 * (Vec3, Vec4, Mat4) which require specific alignment (16, 32, or 64 bytes).
 *
 * Example usage:
 * @code
 * Allocator* allocator = getDefaultAllocator();
 *
 * // Allocate single object
 * MyClass* obj = allocator->create<MyClass>(arg1, arg2);
 * allocator->destroy(obj);
 *
 * // Allocate array
 * float* data = allocator->allocateArray<float>(1024);
 * allocator->deallocateArray(data, 1024);
 *
 * // Allocate with specific alignment
 * void* simdData = allocator->allocate(256, 32);  // 32-byte aligned
 * allocator->deallocate(simdData, 256);
 * @endcode
 */
class Allocator {
public:
    virtual ~Allocator() = default;

    /**
     * @brief Allocate memory with specified size and alignment
     *
     * @param size Size in bytes to allocate (must be > 0)
     * @param alignment Alignment requirement in bytes (must be power of 2)
     * @return Pointer to allocated memory, or nullptr on failure
     *
     * @note The returned pointer must be aligned to at least 'alignment' bytes
     * @note Alignment must be a power of 2
     * @note The allocated memory is not initialized
     */
    virtual void* allocate(size_t size, size_t alignment) = 0;

    /**
     * @brief Deallocate previously allocated memory
     *
     * @param ptr Pointer to memory to deallocate (must be from this allocator)
     * @param size Size in bytes that was originally allocated
     *
     * @note ptr must have been allocated by this allocator
     * @note size must match the size passed to allocate()
     * @note Passing nullptr is safe (no-op)
     */
    virtual void deallocate(void* ptr, size_t size) = 0;

    /**
     * @brief Get total currently allocated size
     *
     * @return Total number of bytes currently allocated
     *
     * @note This includes alignment padding
     * @note Useful for tracking memory usage and detecting leaks
     */
    virtual size_t getAllocatedSize() const = 0;

    /**
     * @brief Construct an object of type T with given arguments
     *
     * Allocates memory for type T with proper alignment, then constructs
     * the object in-place using the provided arguments.
     *
     * @tparam T Type to construct
     * @tparam Args Constructor argument types
     * @param args Arguments to forward to T's constructor
     * @return Pointer to constructed object, or nullptr on allocation failure
     *
     * Example:
     * @code
     * auto* obj = allocator->create<MyClass>(arg1, arg2, arg3);
     * @endcode
     */
    template <typename T, typename... Args>
    T* create(Args&&... args) {
        void* ptr = allocate(sizeof(T), alignof(T));
        if (!ptr) {
            return nullptr;
        }

        // Use placement new to construct object
        return new (ptr) T(std::forward<Args>(args)...);
    }

    /**
     * @brief Destroy an object previously created with create<T>()
     *
     * Calls the destructor for the object and deallocates its memory.
     *
     * @tparam T Type of object to destroy
     * @param ptr Pointer to object (must have been created by this allocator)
     *
     * @note Passing nullptr is safe (no-op)
     *
     * Example:
     * @code
     * allocator->destroy(obj);
     * @endcode
     */
    template <typename T>
    void destroy(T* ptr) {
        if (!ptr) {
            return;
        }

        // Call destructor
        if constexpr (!std::is_trivially_destructible_v<T>) {
            ptr->~T();
        }

        // Deallocate memory
        deallocate(ptr, sizeof(T));
    }

    /**
     * @brief Allocate an array of objects of type T
     *
     * Allocates memory for an array of 'count' objects of type T.
     * The memory is not initialized (no constructors are called).
     *
     * @tparam T Element type
     * @param count Number of elements to allocate
     * @return Pointer to allocated array, or nullptr on failure
     *
     * @note Use allocateArrayWithInit() if you need default construction
     * @note For non-POD types, you must manually construct elements
     *
     * Example:
     * @code
     * float* data = allocator->allocateArray<float>(1024);
     * @endcode
     */
    template <typename T>
    T* allocateArray(size_t count) {
        if (count == 0) {
            return nullptr;
        }

        const size_t totalSize = sizeof(T) * count;
        void* ptr = allocate(totalSize, alignof(T));

        return static_cast<T*>(ptr);
    }

    /**
     * @brief Deallocate an array previously allocated with allocateArray<T>()
     *
     * @tparam T Element type
     * @param ptr Pointer to array (must have been allocated by this allocator)
     * @param count Number of elements in the array
     *
     * @note This does NOT call destructors - you must do that manually
     * @note Passing nullptr is safe (no-op)
     *
     * Example:
     * @code
     * allocator->deallocateArray(data, 1024);
     * @endcode
     */
    template <typename T>
    void deallocateArray(T* ptr, size_t count) {
        if (!ptr || count == 0) {
            return;
        }

        deallocate(ptr, sizeof(T) * count);
    }

    /**
     * @brief Allocate and default-construct an array of objects
     *
     * Allocates memory for an array of 'count' objects of type T,
     * then default-constructs each element.
     *
     * @tparam T Element type
     * @param count Number of elements to allocate
     * @return Pointer to allocated and initialized array, or nullptr on failure
     *
     * Example:
     * @code
     * MyClass* objects = allocator->allocateArrayWithInit<MyClass>(100);
     * @endcode
     */
    template <typename T>
    T* allocateArrayWithInit(size_t count) {
        T* ptr = allocateArray<T>(count);
        if (!ptr) {
            return nullptr;
        }

        // Default-construct each element
        for (size_t i = 0; i < count; ++i) {
            new (&ptr[i]) T();
        }

        return ptr;
    }

    /**
     * @brief Destroy and deallocate an array created with allocateArrayWithInit()
     *
     * Calls the destructor for each element, then deallocates the array.
     *
     * @tparam T Element type
     * @param ptr Pointer to array (must have been allocated by this allocator)
     * @param count Number of elements in the array
     *
     * @note Passing nullptr is safe (no-op)
     *
     * Example:
     * @code
     * allocator->destroyArray(objects, 100);
     * @endcode
     */
    template <typename T>
    void destroyArray(T* ptr, size_t count) {
        if (!ptr || count == 0) {
            return;
        }

        // Call destructor for each element (in reverse order)
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (size_t i = count; i > 0; --i) {
                ptr[i - 1].~T();
            }
        }

        // Deallocate memory
        deallocateArray(ptr, count);
    }
};

/**
 * @brief Cross-platform aligned memory allocation
 *
 * Allocates memory aligned to the specified boundary. The alignment must be
 * a power of 2 and typically should be at least sizeof(void*).
 *
 * @param size Size in bytes to allocate
 * @param alignment Alignment requirement in bytes (must be power of 2)
 * @return Pointer to aligned memory, or nullptr on failure
 *
 * @note Use alignedFree() to deallocate memory allocated with this function
 * @note The returned memory is not initialized
 */
void* alignedAlloc(size_t size, size_t alignment);

/**
 * @brief Free memory allocated with alignedAlloc()
 *
 * @param ptr Pointer to memory to free (must be from alignedAlloc())
 *
 * @note Passing nullptr is safe (no-op)
 */
void alignedFree(void* ptr);

/**
 * @brief Get the default global allocator
 *
 * Returns a pointer to the default allocator used throughout the engine.
 * The default allocator uses the system's malloc/free with alignment support.
 *
 * @return Pointer to the default allocator (never nullptr)
 *
 * @note The default allocator is thread-safe
 * @note The returned pointer is valid for the entire program lifetime
 *
 * Example:
 * @code
 * Allocator* allocator = getDefaultAllocator();
 * auto* obj = allocator->create<MyClass>();
 * @endcode
 */
Allocator* getDefaultAllocator();

/**
 * @brief Set a custom default allocator
 *
 * Replaces the default allocator with a custom implementation.
 * The previous allocator is returned.
 *
 * @param allocator New default allocator (must not be nullptr)
 * @return Previous default allocator
 *
 * @warning The caller is responsible for ensuring the allocator remains
 *          valid for the entire time it is set as the default
 * @warning This function is NOT thread-safe - call only during initialization
 *
 * Example:
 * @code
 * MyCustomAllocator customAllocator;
 * Allocator* previous = setDefaultAllocator(&customAllocator);
 * // ... use custom allocator ...
 * setDefaultAllocator(previous);  // Restore
 * @endcode
 */
Allocator* setDefaultAllocator(Allocator* allocator);

}  // namespace axiom::memory
