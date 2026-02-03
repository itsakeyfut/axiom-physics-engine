#include <axiom/core/threading/job_system.hpp>
#include <axiom/core/profiler.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <random>

namespace axiom::core::threading {

thread_local uint32_t JobSystem::threadIndex_ = UINT32_MAX;

JobSystem::JobSystem() {
    jobPool_ = std::make_unique<Job[]>(MAX_JOBS);
}

JobSystem::~JobSystem() {
    shutdown();
}

JobSystem& JobSystem::instance() {
    static JobSystem instance;
    return instance;
}

void JobSystem::initialize(uint32_t numThreads, JobErrorCallback errorCallback) {
    if (running_.load(std::memory_order_acquire)) {
        shutdown();
    }

    errorCallback_ = errorCallback;

    // Determine thread count
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            numThreads = 4;
        }
    }

    workerCount_ = numThreads;
    running_.store(true, std::memory_order_release);

    // Create queues
    queues_.clear();
    queues_.reserve(workerCount_);
    for (uint32_t i = 0; i < workerCount_; ++i) {
        queues_.push_back(std::make_unique<WorkStealingQueue<uint32_t>>(2048));
    }

    // Start workers
    workers_.clear();
    workers_.reserve(workerCount_);
    for (uint32_t i = 0; i < workerCount_; ++i) {
        workers_.emplace_back([this, i]() { workerMain(i); });
    }
}

void JobSystem::shutdown() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(false, std::memory_order_release);
    wakeWorkers();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
    queues_.clear();
    finishedJobs_.clear();
    nextFreeJob_.store(1, std::memory_order_relaxed);
    activeJobs_.store(0, std::memory_order_relaxed);
}

void JobSystem::beginFrame() {
    AXIOM_PROFILE_SCOPE("JobSystem::beginFrame");

    // Wait for all active jobs to complete before cleanup
    while (activeJobs_.load(std::memory_order_acquire) > 0) {
        std::this_thread::yield();
    }

    cleanupFinishedJobs();
    currentGeneration_++;
}

JobHandle JobSystem::createJob(JobFunc func, const char* debugName) {
    AXIOM_PROFILE_SCOPE("JobSystem::createJob");

    JobHandle handle = allocateJob();
    if (!handle.isValid()) {
        if (errorCallback_) {
            errorCallback_("Failed to allocate job: pool exhausted");
        }
        return JobHandle{};
    }

    Job* job = getJobPtr(handle);
    if (!job) {
        return JobHandle{};
    }

    job->func = std::move(func);
    job->debugName = debugName;
    job->state.store(JobState::Created, std::memory_order_release);

    #ifdef AXIOM_ENABLE_PROFILING
    job->createTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    #endif

    return handle;
}

JobHandle JobSystem::createParallelFor(uint32_t count, ParallelForFunc func,
                                       uint32_t batchSize, const char* debugName) {
    AXIOM_PROFILE_SCOPE("JobSystem::createParallelFor");

    if (count == 0 || batchSize == 0) {
        return JobHandle{};
    }

    uint32_t numBatches = (count + batchSize - 1) / batchSize;

    if (numBatches == 1) {
        // Single batch: create simple job
        return createJob([func, count]() {
            func(0, count, threadIndex_);
        }, debugName);
    }

    // Create parent job
    JobHandle parent = allocateJob();
    if (!parent.isValid()) {
        return JobHandle{};
    }

    Job* parentJob = getJobPtr(parent);
    if (!parentJob) {
        return JobHandle{};
    }

    parentJob->debugName = debugName;
    parentJob->unfinishedChildren.store(numBatches, std::memory_order_relaxed);
    parentJob->state.store(JobState::Created, std::memory_order_release);

    // Create child jobs
    for (uint32_t batch = 0; batch < numBatches; ++batch) {
        uint32_t begin = batch * batchSize;
        uint32_t end = std::min(begin + batchSize, count);

        JobHandle child = createChildJob(parent, [func, begin, end]() {
            func(begin, end, threadIndex_);
        }, debugName);

        if (child.isValid()) {
            schedule(child);
        } else {
            // Failed to create child: decrement count
            parentJob->unfinishedChildren.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    return parent;
}

JobHandle JobSystem::createChildJob(JobHandle parent, JobFunc func, const char* debugName) {
    JobHandle handle = allocateJob();
    if (!handle.isValid()) {
        return JobHandle{};
    }

    Job* job = getJobPtr(handle);
    if (!job) {
        return JobHandle{};
    }

    job->func = std::move(func);
    job->debugName = debugName;
    job->parentIndex = parent.index;
    job->state.store(JobState::Created, std::memory_order_release);

    return handle;
}

void JobSystem::schedule(JobHandle handle) {
    if (!handle.isValid()) {
        return;
    }

    Job* job = getJobPtr(handle);
    if (!job || job->generation != handle.generation) {
        return; // Stale handle
    }

    JobState expected = JobState::Created;
    if (!job->state.compare_exchange_strong(expected, JobState::Scheduled,
                                            std::memory_order_acq_rel)) {
        return; // Already scheduled or running
    }

    #ifdef AXIOM_ENABLE_PROFILING
    job->scheduleTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    #endif

    activeJobs_.fetch_add(1, std::memory_order_relaxed);

    // Add to queue
    uint32_t threadIdx = threadIndex_;
    if (threadIdx == UINT32_MAX || threadIdx >= workerCount_) {
        static thread_local std::mt19937 rng{std::random_device{}()};
        threadIdx = static_cast<uint32_t>(rng() % workerCount_);
    }

    queues_[threadIdx]->push(handle.index);

    // Wake workers more aggressively
    wakeCondition_.notify_all();
}

void JobSystem::scheduleAndWait(JobHandle handle) {
    schedule(handle);
    wait(handle);
}

void JobSystem::wait(JobHandle handle) {
    if (!handle.isValid()) {
        return;
    }

    AXIOM_PROFILE_SCOPE("JobSystem::wait");

    Job* job = getJobPtr(handle);
    if (!job || job->generation != handle.generation) {
        return;
    }

    uint32_t threadIdx = threadIndex_;

    // Phase 1: Spin briefly (low latency for short jobs)
    constexpr int spinIterations = 100;
    for (int i = 0; i < spinIterations; ++i) {
        if (job->state.load(std::memory_order_acquire) == JobState::Finished) {
            return;
        }

        // Help execute jobs while spinning (all threads, including main thread)
        Job* workJob = getJob(threadIdx);
        if (workJob) {
            executeJob(workJob, threadIdx);
        }

        std::this_thread::yield();
    }

    // Phase 2: Help process jobs while waiting (efficient and prevents deadlock)
    auto startTime = std::chrono::steady_clock::now();
    constexpr auto deadlockTimeout = std::chrono::seconds(60); // 60 seconds timeout

    while (job->state.load(std::memory_order_acquire) != JobState::Finished) {
        // Check for potential deadlock
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (elapsed > deadlockTimeout) {
            // Log diagnostic information
            JobState state = job->state.load(std::memory_order_acquire);
            uint32_t unfinished = job->unfinishedChildren.load(std::memory_order_acquire);
            uint32_t active = activeJobs_.load(std::memory_order_relaxed);

            char buffer[512];
            snprintf(buffer, sizeof(buffer),
                "\n[DEADLOCK] JobSystem::wait() timeout - possible deadlock detected\n"
                "  Job state: %d (0=Free, 1=Created, 2=Scheduled, 3=Running, 4=Finished)\n"
                "  Unfinished children: %u\n"
                "  Active jobs: %u\n"
                "  Debug name: %s\n",
                static_cast<int>(state), unfinished, active,
                job->debugName ? job->debugName : "unknown");

            fprintf(stderr, "%s", buffer);

            if (errorCallback_) {
                errorCallback_(buffer);
            }

            // In production, we'd return here, but for testing, we assert to catch the bug
            assert(false && "JobSystem deadlock detected");
            return;
        }

        // Help execute jobs while waiting (all threads, including main thread)
        // This prevents deadlock when all worker threads are blocked waiting
        Job* workJob = getJob(threadIdx);
        if (workJob) {
            executeJob(workJob, threadIdx);
            continue; // Check job completion immediately after helping
        }

        // No work available: use condition variable with minimal lock time
        // This approach balances responsiveness with worker thread access
        {
            std::unique_lock<std::mutex> lock(wakeMutex_);
            // Double-check job state while holding lock to avoid missed notifications
            if (job->state.load(std::memory_order_acquire) == JobState::Finished) {
                return;
            }
            // Wait briefly, then release lock to let workers make progress
            wakeCondition_.wait_for(lock, std::chrono::milliseconds(1));
        }
        // Lock is released here - workers can now acquire it
    }
}

void JobSystem::waitAll(std::span<const JobHandle> jobs) {
    for (auto handle : jobs) {
        wait(handle);
    }
}

bool JobSystem::isFinished(JobHandle handle) const {
    if (!handle.isValid()) {
        return true;
    }

    Job* job = getJobPtr(handle);
    if (!job || job->generation != handle.generation) {
        return true; // Stale handle considered finished
    }

    return job->state.load(std::memory_order_acquire) == JobState::Finished;
}

void JobSystem::workerMain(uint32_t threadIndex) {
    threadIndex_ = threadIndex;

    AXIOM_PROFILE_SCOPE("JobSystem::Worker");

    while (running_.load(std::memory_order_acquire)) {
        Job* job = getJob(threadIndex);

        if (job) {
            executeJob(job, threadIndex);
        } else {
            // No work: yield briefly then wait
            std::this_thread::yield();

            // Check running_ again before acquiring lock to ensure shutdown responsiveness
            if (!running_.load(std::memory_order_acquire)) {
                break;
            }

            std::unique_lock<std::mutex> lock(wakeMutex_);
            if (activeJobs_.load(std::memory_order_relaxed) == 0) {
                wakeCondition_.wait_for(lock, std::chrono::milliseconds(10));
            }
        }
    }

    // Process remaining jobs
    while (Job* job = getJob(threadIndex)) {
        executeJob(job, threadIndex);
    }
}

void JobSystem::executeJob(Job* job, [[maybe_unused]] uint32_t threadIndex) {
    if (!job) {
        return;
    }

    AXIOM_PROFILE_SCOPE(job->debugName ? job->debugName : "Job");

    JobState expected = JobState::Scheduled;
    if (!job->state.compare_exchange_strong(expected, JobState::Running,
                                            std::memory_order_acq_rel)) {
        return; // Already running or finished
    }

    #ifdef AXIOM_ENABLE_PROFILING
    job->startTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    #endif

    // Execute
    try {
        if (job->func) {
            job->func();
        }
    } catch (const std::exception& e) {
        if (errorCallback_) {
            errorCallback_(e.what());
        }
    } catch (...) {
        if (errorCallback_) {
            errorCallback_("Unknown exception in job");
        }
    }

    #ifdef AXIOM_ENABLE_PROFILING
    job->endTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    #endif

    // Find job index
    uint32_t jobIndex = static_cast<uint32_t>(job - jobPool_.get());
    finishJob(job, jobIndex);
}

JobSystem::Job* JobSystem::getJob(uint32_t threadIndex) {
    // Try own queue first
    if (threadIndex < queues_.size()) {
        auto jobIndex = queues_[threadIndex]->pop();
        if (jobIndex.has_value()) {
            uint32_t idx = jobIndex.value();
            if (idx > 0 && idx < MAX_JOBS) {
                return &jobPool_[idx];
            } else {
                // Invalid job index detected - this indicates a serious bug
                fprintf(stderr, "[ERROR] getJob: Invalid job index from own queue: %u (thread %u)\n",
                    idx, threadIndex);
            }
        }
    }

    // Steal from others
    static thread_local std::mt19937 rng{std::random_device{}()};
    uint32_t start = static_cast<uint32_t>(rng() % workerCount_);

    for (uint32_t i = 0; i < workerCount_; ++i) {
        uint32_t victimIdx = (start + i) % workerCount_;
        if (victimIdx == threadIndex) {
            continue;
        }

        auto jobIndex = queues_[victimIdx]->steal();
        if (jobIndex.has_value()) {
            uint32_t idx = jobIndex.value();
            if (idx > 0 && idx < MAX_JOBS) {
                return &jobPool_[idx];
            } else {
                // Invalid job index detected from stealing
                fprintf(stderr, "[ERROR] getJob: Invalid job index stolen from queue %u: %u (thread %u)\n",
                    victimIdx, idx, threadIndex);
            }
        }
    }

    return nullptr;
}

JobHandle JobSystem::allocateJob() {
    // Use thread-local start position to reduce contention
    static thread_local uint32_t tlsStart = 1;
    uint32_t start = tlsStart;

    // Search pool from thread's last position
    // Only allocate Free jobs - Finished jobs may still be in queues
    for (uint32_t i = 0; i < MAX_JOBS; ++i) {
        uint32_t index = start + i;
        if (index >= MAX_JOBS) index = 1 + (index - MAX_JOBS);
        if (index == 0) continue;

        Job& job = jobPool_[index];

        // Try to claim Free job
        JobState expected = JobState::Free;
        if (job.state.compare_exchange_strong(expected, JobState::Created,
                                              std::memory_order_acq_rel)) {
            job.generation = currentGeneration_;
            job.parentIndex = 0;
            job.unfinishedChildren.store(0, std::memory_order_relaxed);
            job.func = nullptr;
            job.debugName = nullptr;

            tlsStart = index + 1;
            return JobHandle{index, currentGeneration_};
        }
    }

    return JobHandle{}; // Pool exhausted
}

JobSystem::Job* JobSystem::getJobPtr(JobHandle handle) const {
    if (!handle.isValid() || handle.index >= MAX_JOBS) {
        return nullptr;
    }
    return &jobPool_[handle.index];
}

void JobSystem::finishJob(Job* job, uint32_t jobIndex) {
    // If job has unfinished children, don't finish yet
    // The last child will call finishJob again via notifyJobFinished
    if (job->unfinishedChildren.load(std::memory_order_acquire) > 0) {
        return;
    }

    // Atomically transition to Finished and get previous state
    JobState prevState = job->state.exchange(JobState::Finished, std::memory_order_acq_rel);

    // Only decrement activeJobs if the job was actually scheduled
    // (Parent jobs may finish via notifyJobFinished before being scheduled)
    if (prevState != JobState::Created) {
        activeJobs_.fetch_sub(1, std::memory_order_relaxed);
    }

    // Notify parent
    if (job->parentIndex != 0) {
        notifyJobFinished(job->parentIndex);
    }

    // Add to finished list for later cleanup/reuse
    {
        std::lock_guard<std::mutex> lock(finishedMutex_);
        finishedJobs_.push_back(jobIndex);
    }

    wakeWorkers();
}

void JobSystem::notifyJobFinished(uint32_t parentIndex) {
    if (parentIndex == 0 || parentIndex >= MAX_JOBS) {
        return;
    }

    Job& parent = jobPool_[parentIndex];
    uint32_t remaining = parent.unfinishedChildren.fetch_sub(1, std::memory_order_acq_rel) - 1;

    if (remaining == 0) {
        // All children finished: mark parent as finished
        finishJob(&parent, parentIndex);
    }
}

void JobSystem::cleanupFinishedJobs() {
    std::lock_guard<std::mutex> lock(finishedMutex_);

    // Free all finished jobs at frame boundaries
    // This ensures jobs are not recycled while still in queues
    for (uint32_t index : finishedJobs_) {
        Job& job = jobPool_[index];

        // Only free if still in Finished state (not already recycled)
        JobState expected = JobState::Finished;
        if (job.state.compare_exchange_strong(expected, JobState::Free,
                                               std::memory_order_acq_rel)) {
            job.func = {}; // Release captured resources
        }
    }

    finishedJobs_.clear();
}

void JobSystem::wakeWorkers() {
    wakeCondition_.notify_all();
}

} // namespace axiom::core::threading
