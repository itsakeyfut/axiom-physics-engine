#pragma once

#include <axiom/core/threading/work_stealing_queue.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace axiom::core::threading {

using JobFunc = std::function<void()>;
using ParallelForFunc = std::function<void(uint32_t begin, uint32_t end, uint32_t threadIndex)>;
using JobErrorCallback = std::function<void(const char* errorMessage)>;

/**
 * @brief Handle to a job with generation-based validation
 *
 * Uses generation number to detect stale handles (ABA problem prevention)
 */
struct JobHandle {
    uint32_t index{0};      ///< Index into job pool
    uint32_t generation{0}; ///< Generation number for validation

    bool isValid() const { return index != 0; }
    bool operator==(const JobHandle& other) const {
        return index == other.index && generation == other.generation;
    }
    bool operator!=(const JobHandle& other) const { return !(*this == other); }
};

/**
 * @brief High-performance job system with work-stealing
 *
 * Key features:
 * - Hybrid job allocation (pool for small jobs, dynamic for large)
 * - Hybrid waiting (spin then block with condition variable)
 * - Generation-based handle validation
 * - Automatic cleanup at frame boundaries
 * - Tracy profiling integration
 * - Zero-overhead when profiling disabled
 *
 * Thread-safe after initialization.
 */
class JobSystem {
public:
    static JobSystem& instance();

    /**
     * @brief Initialize job system
     * @param numThreads Worker thread count (0 = auto-detect)
     * @param errorCallback Optional error handler
     */
    void initialize(uint32_t numThreads = 0, JobErrorCallback errorCallback = nullptr);

    /**
     * @brief Shutdown and wait for all workers
     */
    void shutdown();

    /**
     * @brief Mark frame boundary for cleanup
     *
     * Call once per frame. Cleans up completed jobs from previous frame.
     */
    void beginFrame();

    // Job creation
    JobHandle createJob(JobFunc func, const char* debugName = nullptr);
    JobHandle createParallelFor(uint32_t count, ParallelForFunc func,
                                uint32_t batchSize = 64, const char* debugName = nullptr);
    JobHandle createChildJob(JobHandle parent, JobFunc func, const char* debugName = nullptr);

    // Scheduling
    void schedule(JobHandle job);
    void scheduleAndWait(JobHandle job);

    // Waiting (efficient: spin briefly then block)
    void wait(JobHandle job);
    void waitAll(std::span<const JobHandle> jobs);

    // Queries
    uint32_t getWorkerCount() const { return workerCount_; }
    uint32_t getCurrentThreadIndex() const { return threadIndex_; }
    bool isFinished(JobHandle job) const;

private:
    JobSystem();
    ~JobSystem();

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    // Job states
    enum class JobState : uint8_t {
        Free,       ///< Available for allocation
        Created,    ///< Allocated but not scheduled
        Scheduled,  ///< In queue waiting for execution
        Running,    ///< Currently executing
        Finished    ///< Completed, waiting for cleanup
    };

    // Internal job structure
    struct Job {
        JobFunc func;
        std::atomic<JobState> state{JobState::Free};
        std::atomic<uint32_t> unfinishedChildren{0};
        uint32_t generation{0};
        uint32_t parentIndex{0};
        const char* debugName{nullptr};

        #ifdef AXIOM_ENABLE_PROFILING
        uint64_t createTime{0};
        uint64_t scheduleTime{0};
        uint64_t startTime{0};
        uint64_t endTime{0};
        #endif
    };

    // Worker thread
    void workerMain(uint32_t threadIndex);
    void executeJob(Job* job, uint32_t threadIndex);
    Job* getJob(uint32_t threadIndex);

    // Job management
    JobHandle allocateJob();
    Job* getJobPtr(JobHandle handle) const;
    void finishJob(Job* job, uint32_t jobIndex);
    void cleanupFinishedJobs();

    // Notification
    void notifyJobFinished(uint32_t parentIndex);
    void wakeWorkers();

    // Workers
    std::vector<std::thread> workers_;
    std::vector<std::unique_ptr<WorkStealingQueue<uint32_t>>> queues_;
    static thread_local uint32_t threadIndex_;

    // Job pool (fixed-size array for fast indexing)
    static constexpr uint32_t MAX_JOBS = 8192;
    std::unique_ptr<Job[]> jobPool_;
    std::atomic<uint32_t> nextFreeJob_{1}; // 0 is reserved for invalid
    std::vector<uint32_t> finishedJobs_;   // Cleanup queue
    std::mutex finishedMutex_;

    // Synchronization
    std::mutex wakeMutex_;
    std::condition_variable wakeCondition_;
    std::atomic<uint32_t> activeJobs_{0};

    // State
    std::atomic<bool> running_{false};
    uint32_t workerCount_{0};
    uint32_t currentGeneration_{1};
    JobErrorCallback errorCallback_;
};

} // namespace axiom::core::threading
