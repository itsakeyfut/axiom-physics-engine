#include <axiom/core/threading/job_system.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <numeric>
#include <vector>

using namespace axiom::core::threading;

class JobSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& jobSys = JobSystem::instance();
        jobSys.initialize(4); // Use 4 threads for testing
    }

    void TearDown() override {
        auto& jobSys = JobSystem::instance();
        jobSys.shutdown();
    }
};

TEST_F(JobSystemTest, InitializationAndShutdown) {
    auto& jobSys = JobSystem::instance();
    EXPECT_EQ(jobSys.getWorkerCount(), 4u);
}

TEST_F(JobSystemTest, SimpleJobExecution) {
    auto& jobSys = JobSystem::instance();

    std::atomic<int> counter{0};

    JobHandle job = jobSys.createJob([&counter]() {
        counter.fetch_add(1, std::memory_order_relaxed);
    });

    EXPECT_TRUE(job.isValid());

    jobSys.scheduleAndWait(job);

    EXPECT_EQ(counter.load(), 1);
    EXPECT_TRUE(jobSys.isFinished(job));
}

TEST_F(JobSystemTest, MultipleJobs) {
    auto& jobSys = JobSystem::instance();

    std::atomic<int> counter{0};
    constexpr int numJobs = 8;

    std::vector<JobHandle> jobs;
    for (int i = 0; i < numJobs; ++i) {
        jobs.push_back(jobSys.createJob([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    // Schedule all jobs
    for (auto job : jobs) {
        jobSys.schedule(job);
    }

    // Wait for all jobs
    jobSys.waitAll(jobs);

    EXPECT_EQ(counter.load(), numJobs);

    for (auto job : jobs) {
        EXPECT_TRUE(jobSys.isFinished(job));
    }
}

TEST_F(JobSystemTest, ParallelFor) {
    auto& jobSys = JobSystem::instance();

    constexpr uint32_t count = 100;
    std::vector<std::atomic<int>> values(count);
    for (auto& val : values) {
        val.store(0, std::memory_order_relaxed);
    }

    JobHandle job = jobSys.createParallelFor(count,
        [&values](uint32_t begin, uint32_t end, [[maybe_unused]] uint32_t threadIdx) {
            for (uint32_t i = begin; i < end; ++i) {
                values[i].fetch_add(1, std::memory_order_relaxed);
            }
        },
        64); // Batch size

    jobSys.scheduleAndWait(job);

    // Verify all values were incremented
    for (uint32_t i = 0; i < count; ++i) {
        EXPECT_EQ(values[i].load(), 1) << "Index " << i << " was not incremented";
    }
}

TEST_F(JobSystemTest, ParallelForWithSmallBatch) {
    auto& jobSys = JobSystem::instance();

    constexpr uint32_t count = 50;
    std::vector<int> values(count, 0);
    std::atomic<int> totalSum{0};

    JobHandle job = jobSys.createParallelFor(count,
        [&values, &totalSum](uint32_t begin, uint32_t end, [[maybe_unused]] uint32_t threadIdx) {
            int localSum = 0;
            for (uint32_t i = begin; i < end; ++i) {
                values[i] = static_cast<int>(i);
                localSum += static_cast<int>(i);
            }
            totalSum.fetch_add(localSum, std::memory_order_relaxed);
        },
        25); // Batch size that creates only 2 batches

    jobSys.scheduleAndWait(job);

    // Expected sum: 0 + 1 + 2 + ... + 49
    int expectedSum = (count * (count - 1)) / 2;
    EXPECT_EQ(totalSum.load(), expectedSum);
}

// TODO: Implement createJobWithDependencies for explicit dependency management
// TEST_F(JobSystemTest, JobDependencies) { ... }
// TEST_F(JobSystemTest, MultipleDependencies) { ... }

TEST_F(JobSystemTest, NoRaceConditions) {
    auto& jobSys = JobSystem::instance();

    std::atomic<int> counter{0};
    constexpr int numJobs = 10;
    constexpr int incrementsPerJob = 100;

    std::vector<JobHandle> jobs;
    for (int i = 0; i < numJobs; ++i) {
        jobs.push_back(jobSys.createJob([&counter]() {
            for (int j = 0; j < incrementsPerJob; ++j) {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        }));
    }

    for (auto job : jobs) {
        jobSys.schedule(job);
    }

    jobSys.waitAll(jobs);

    EXPECT_EQ(counter.load(), numJobs * incrementsPerJob);
}

TEST_F(JobSystemTest, WorkStealingBalance) {
    auto& jobSys = JobSystem::instance();

    constexpr int numJobs = 10;
    std::atomic<int> counter{0};

    // Create jobs that take varying amounts of time
    std::vector<JobHandle> jobs;
    for (int i = 0; i < numJobs; ++i) {
        jobs.push_back(jobSys.createJob([&counter, i]() {
            // Variable work to test load balancing
            int work = 0;
            for (int j = 0; j < (i * 10); ++j) {
                work += j;
            }
            counter.fetch_add(work, std::memory_order_relaxed);
        }));
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (auto job : jobs) {
        jobSys.schedule(job);
    }
    jobSys.waitAll(jobs);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Just verify it completes in reasonable time
    EXPECT_LT(duration.count(), 5000) << "Work-stealing may not be working efficiently";
}

TEST_F(JobSystemTest, InvalidJobHandle) {
    auto& jobSys = JobSystem::instance();

    JobHandle invalid{0};
    EXPECT_FALSE(invalid.isValid());

    // These should not crash
    jobSys.schedule(invalid);
    jobSys.wait(invalid);
    EXPECT_TRUE(jobSys.isFinished(invalid));
}

TEST_F(JobSystemTest, ScheduleAndWait) {
    auto& jobSys = JobSystem::instance();

    std::atomic<bool> executed{false};

    JobHandle job = jobSys.createJob([&executed]() {
        executed.store(true, std::memory_order_release);
    });

    jobSys.scheduleAndWait(job);

    EXPECT_TRUE(executed.load(std::memory_order_acquire));
    EXPECT_TRUE(jobSys.isFinished(job));
}

TEST_F(JobSystemTest, EmptyParallelFor) {
    auto& jobSys = JobSystem::instance();

    // Zero count should return invalid handle
    JobHandle job = jobSys.createParallelFor(0, [](uint32_t, uint32_t, uint32_t) {});
    EXPECT_FALSE(job.isValid());

    // Zero batch size should return invalid handle
    job = jobSys.createParallelFor(100, [](uint32_t, uint32_t, uint32_t) {}, 0);
    EXPECT_FALSE(job.isValid());
}

TEST_F(JobSystemTest, SingleBatchParallelFor) {
    auto& jobSys = JobSystem::instance();

    std::atomic<int> batchCount{0};

    JobHandle job = jobSys.createParallelFor(10,
        [&batchCount](uint32_t begin, uint32_t end, [[maybe_unused]] uint32_t threadIdx) {
            batchCount.fetch_add(1, std::memory_order_relaxed);
            EXPECT_EQ(begin, 0u);
            EXPECT_EQ(end, 10u);
        },
        100); // Batch size larger than count

    jobSys.scheduleAndWait(job);

    // Should execute as a single batch
    EXPECT_EQ(batchCount.load(), 1);
}

TEST_F(JobSystemTest, StressTest) {
    auto& jobSys = JobSystem::instance();

    constexpr int numIterations = 2;
    constexpr int jobsPerIteration = 5;

    for (int iter = 0; iter < numIterations; ++iter) {
        std::atomic<int> counter{0};
        std::vector<JobHandle> jobs;

        for (int i = 0; i < jobsPerIteration; ++i) {
            jobs.push_back(jobSys.createJob([&counter]() {
                counter.fetch_add(1, std::memory_order_relaxed);
            }));
        }

        for (auto job : jobs) {
            jobSys.schedule(job);
        }

        jobSys.waitAll(jobs);

        EXPECT_EQ(counter.load(), jobsPerIteration);
    }
}

TEST_F(JobSystemTest, ThreadIndexRetrieval) {
    auto& jobSys = JobSystem::instance();

    std::vector<std::atomic<bool>> threadUsed(jobSys.getWorkerCount());
    for (auto& used : threadUsed) {
        used.store(false, std::memory_order_relaxed);
    }

    constexpr int numJobs = 8;
    std::vector<JobHandle> jobs;

    for (int i = 0; i < numJobs; ++i) {
        jobs.push_back(jobSys.createJob([&jobSys, &threadUsed]() {
            uint32_t idx = jobSys.getCurrentThreadIndex();
            if (idx < threadUsed.size()) {
                threadUsed[idx].store(true, std::memory_order_relaxed);
            }
        }));
    }

    for (auto job : jobs) {
        jobSys.schedule(job);
    }
    jobSys.waitAll(jobs);

    // At least some threads should have been used
    int threadsUsed = 0;
    for (const auto& used : threadUsed) {
        if (used.load(std::memory_order_relaxed)) {
            ++threadsUsed;
        }
    }

    EXPECT_GT(threadsUsed, 0);
}
