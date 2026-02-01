#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>

namespace axiom::core::threading {

/**
 * @brief Lock-free work-stealing queue (Chase-Lev deque)
 *
 * Optimized for:
 * - Owner thread: push/pop from bottom (LIFO for cache locality)
 * - Thief threads: steal from top (FIFO for load balancing)
 *
 * @tparam T Type of items (must be trivially copyable)
 */
template <typename T>
class WorkStealingQueue {
public:
    explicit WorkStealingQueue(uint32_t capacity = 4096);
    ~WorkStealingQueue();

    WorkStealingQueue(const WorkStealingQueue&) = delete;
    WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;

    void push(T item);
    std::optional<T> pop();
    std::optional<T> steal();

    bool isEmpty() const;
    uint32_t size() const;

private:
    struct Array {
        uint32_t capacity;
        std::unique_ptr<T[]> data;

        explicit Array(uint32_t cap);
        T get(uint32_t index) const;
        void put(uint32_t index, T item);
    };

    void grow(int64_t bottom, int64_t top);

    std::atomic<int64_t> top_;
    std::atomic<int64_t> bottom_;
    std::atomic<Array*> array_;
    std::unique_ptr<Array> garbage_;
};

// Implementation

template <typename T>
WorkStealingQueue<T>::Array::Array(uint32_t cap)
    : capacity(cap), data(std::make_unique<T[]>(cap)) {
}

template <typename T>
T WorkStealingQueue<T>::Array::get(uint32_t index) const {
    return data[index % capacity];
}

template <typename T>
void WorkStealingQueue<T>::Array::put(uint32_t index, T item) {
    data[index % capacity] = item;
}

template <typename T>
WorkStealingQueue<T>::WorkStealingQueue(uint32_t capacity)
    : top_(0), bottom_(0), garbage_(nullptr) {
    // Ensure power of 2
    uint32_t powerOf2 = 1;
    while (powerOf2 < capacity) {
        powerOf2 <<= 1;
    }
    array_.store(new Array(powerOf2), std::memory_order_relaxed);
}

template <typename T>
WorkStealingQueue<T>::~WorkStealingQueue() {
    delete array_.load();
}

template <typename T>
void WorkStealingQueue<T>::push(T item) {
    int64_t b = bottom_.load(std::memory_order_relaxed);
    int64_t t = top_.load(std::memory_order_acquire);
    Array* a = array_.load(std::memory_order_relaxed);

    if (b - t >= static_cast<int64_t>(a->capacity)) {
        grow(b, t);
        a = array_.load(std::memory_order_relaxed);
    }

    a->put(static_cast<uint32_t>(b), item);
    std::atomic_thread_fence(std::memory_order_release);
    bottom_.store(b + 1, std::memory_order_relaxed);
}

template <typename T>
std::optional<T> WorkStealingQueue<T>::pop() {
    int64_t b = bottom_.load(std::memory_order_relaxed) - 1;
    Array* a = array_.load(std::memory_order_relaxed);
    bottom_.store(b, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    int64_t t = top_.load(std::memory_order_relaxed);

    if (t <= b) {
        T item = a->get(static_cast<uint32_t>(b));
        if (t == b) {
            if (!top_.compare_exchange_strong(t, t + 1,
                                              std::memory_order_seq_cst,
                                              std::memory_order_relaxed)) {
                bottom_.store(b + 1, std::memory_order_relaxed);
                return std::nullopt;
            }
            bottom_.store(b + 1, std::memory_order_relaxed);
        }
        return item;
    } else {
        bottom_.store(b + 1, std::memory_order_relaxed);
        return std::nullopt;
    }
}

template <typename T>
std::optional<T> WorkStealingQueue<T>::steal() {
    int64_t t = top_.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    int64_t b = bottom_.load(std::memory_order_acquire);

    if (t < b) {
        Array* a = array_.load(std::memory_order_consume);
        T item = a->get(static_cast<uint32_t>(t));
        if (!top_.compare_exchange_strong(t, t + 1,
                                          std::memory_order_seq_cst,
                                          std::memory_order_relaxed)) {
            return std::nullopt;
        }
        return item;
    }
    return std::nullopt;
}

template <typename T>
bool WorkStealingQueue<T>::isEmpty() const {
    int64_t b = bottom_.load(std::memory_order_relaxed);
    int64_t t = top_.load(std::memory_order_relaxed);
    return b <= t;
}

template <typename T>
uint32_t WorkStealingQueue<T>::size() const {
    int64_t b = bottom_.load(std::memory_order_relaxed);
    int64_t t = top_.load(std::memory_order_relaxed);
    int64_t sz = b - t;
    return sz > 0 ? static_cast<uint32_t>(sz) : 0;
}

template <typename T>
void WorkStealingQueue<T>::grow(int64_t bottom, int64_t top) {
    Array* oldArray = array_.load(std::memory_order_relaxed);
    uint32_t newCapacity = oldArray->capacity * 2;
    Array* newArray = new Array(newCapacity);

    for (int64_t i = top; i < bottom; ++i) {
        newArray->put(static_cast<uint32_t>(i), oldArray->get(static_cast<uint32_t>(i)));
    }

    array_.store(newArray, std::memory_order_release);
    garbage_.reset(oldArray);
}

} // namespace axiom::core::threading
