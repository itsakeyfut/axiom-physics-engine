/**
 * @file profiling_example.cpp
 * @brief Example demonstrating profiling infrastructure usage
 *
 * This example shows how to use the profiling macros in Axiom Physics Engine.
 * When built with AXIOM_ENABLE_PROFILING=ON, this will generate profiling data
 * that can be viewed in Tracy Profiler.
 *
 * Build:
 *   cmake --preset windows-relwithdebinfo -DAXIOM_ENABLE_PROFILING=ON
 *   cmake --build build/windows-relwithdebinfo
 *
 * Run:
 *   ./build/windows-relwithdebinfo/bin/profiling_example
 *
 * View Results:
 *   1. Launch Tracy server application
 *   2. Run this example
 *   3. Tracy will automatically connect and display profiling data
 */

#include <axiom/core/profiler.hpp>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

/**
 * @brief Simulated physics object
 */
struct PhysicsObject {
    float x, y, z;
    float vx, vy, vz;

    void integrate(float dt) {
        AXIOM_PROFILE_FUNCTION();

        x += vx * dt;
        y += vy * dt;
        z += vz * dt;
    }
};

/**
 * @brief Simulated broadphase collision detection
 */
class Broadphase {
public:
    void update(const std::vector<PhysicsObject>& objects) {
        AXIOM_PROFILE_FUNCTION();

        // Simulate some work
        int pairCount = 0;
        for (size_t i = 0; i < objects.size(); ++i) {
            for (size_t j = i + 1; j < objects.size(); ++j) {
                float dx = objects[i].x - objects[j].x;
                float dy = objects[i].y - objects[j].y;
                float dz = objects[i].z - objects[j].z;
                float distSq = dx * dx + dy * dy + dz * dz;

                if (distSq < 100.0f) {
                    ++pairCount;
                }
            }
        }

        AXIOM_PROFILE_VALUE("BroadphasePairs", static_cast<double>(pairCount));
        (void)pairCount;  // Suppress unused variable warning when profiling disabled
    }
};

/**
 * @brief Simulated narrowphase collision detection
 */
class Narrowphase {
public:
    int detectCollisions(int potentialPairs) {
        AXIOM_PROFILE_FUNCTION();

        // Simulate GJK/EPA work
        std::this_thread::sleep_for(std::chrono::microseconds(500));

        int contactCount = potentialPairs / 2;
        AXIOM_PROFILE_VALUE("ContactCount", contactCount);

        return contactCount;
    }
};

/**
 * @brief Simulated constraint solver
 */
class Solver {
public:
    void solve(int contactCount, float dt) {
        AXIOM_PROFILE_FUNCTION();

        int maxIterations = 10;

        for (int iter = 0; iter < maxIterations; ++iter) {
            AXIOM_PROFILE_SCOPE("SolverIteration");

            // Simulate solving constraints
            volatile float lambda = 0.0f;
            for (int i = 0; i < contactCount; ++i) {
                lambda += dt * 0.1f;
            }

            AXIOM_PROFILE_VALUE("SolverIterations", iter + 1);
        }
    }
};

/**
 * @brief Simulated physics world
 */
class PhysicsWorld {
public:
    PhysicsWorld(size_t numObjects)
        : objects_(numObjects), broadphase_(), narrowphase_(), solver_() {
        // Initialize objects with random positions and velocities
        for (size_t i = 0; i < numObjects; ++i) {
            objects_[i].x = static_cast<float>(i * 10);
            objects_[i].y = 0.0f;
            objects_[i].z = 0.0f;
            objects_[i].vx = 1.0f;
            objects_[i].vy = 0.5f;
            objects_[i].vz = 0.0f;
        }
    }

    void step(float dt) {
        AXIOM_PROFILE_FUNCTION();

        // Broadphase collision detection
        {
            AXIOM_PROFILE_SCOPE("Broadphase");
            broadphase_.update(objects_);
        }

        // Narrowphase collision detection
        int contactCount = 0;
        {
            AXIOM_PROFILE_SCOPE("Narrowphase");
            contactCount = narrowphase_.detectCollisions(
                static_cast<int>(objects_.size() * objects_.size() / 2));
        }

        // Constraint solver
        {
            AXIOM_PROFILE_SCOPE("Solver");
            solver_.solve(contactCount, dt);
        }

        // Integration
        {
            AXIOM_PROFILE_SCOPE("Integration");
            for (auto& obj : objects_) {
                obj.integrate(dt);
            }
        }

        // Mark end of frame
        AXIOM_PROFILE_FRAME();
    }

    size_t getObjectCount() const { return objects_.size(); }

private:
    std::vector<PhysicsObject> objects_;
    Broadphase broadphase_;
    Narrowphase narrowphase_;
    Solver solver_;
};

int main() {
    std::cout << "Axiom Physics Engine - Profiling Example\n";
    std::cout << "=========================================\n\n";

#ifdef AXIOM_ENABLE_PROFILING
    std::cout << "Profiling is ENABLED\n";
    std::cout << "Launch Tracy server to view profiling data.\n\n";
#else
    std::cout << "Profiling is DISABLED\n";
    std::cout << "To enable profiling, build with:\n";
    std::cout
        << "  cmake --preset windows-relwithdebinfo -DAXIOM_ENABLE_PROFILING=ON\n\n";
#endif

    // Create physics world with 50 objects
    constexpr size_t numObjects = 50;
    PhysicsWorld world(numObjects);

    std::cout << "Running physics simulation with " << world.getObjectCount()
              << " objects...\n";
    std::cout << "Simulating 60 frames (1 second at 60 FPS)...\n\n";

    // Simulate 60 frames (1 second at 60 FPS)
    constexpr int numFrames = 60;
    constexpr float dt = 1.0f / 60.0f;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < numFrames; ++frame) {
        AXIOM_PROFILE_SCOPE("MainLoop");

        world.step(dt);

        // Print progress every 10 frames
        if ((frame + 1) % 10 == 0) {
            std::cout << "Frame " << (frame + 1) << "/" << numFrames
                      << " completed\n";
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime)
            .count();

    std::cout << "\nSimulation completed in " << duration << " ms\n";
    std::cout << "Average frame time: "
              << (static_cast<double>(duration) / static_cast<double>(numFrames))
              << " ms\n";

#ifdef AXIOM_ENABLE_PROFILING
    std::cout << "\nProfiling data has been sent to Tracy.\n";
    std::cout << "View results in Tracy Profiler application.\n";

    // Give Tracy time to send data
    std::cout << "Waiting 1 second for Tracy to flush data...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

    return 0;
}
