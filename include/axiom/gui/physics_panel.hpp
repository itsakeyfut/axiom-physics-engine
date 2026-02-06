#pragma once

#include "axiom/debug/physics_debug_draw.hpp"
#include "axiom/math/vec3.hpp"

#include <cstdint>

namespace axiom::gui {

/// Statistics about the physics world state
struct PhysicsWorldStats {
    // Body counts
    uint32_t totalBodies = 0;      ///< Total number of rigid bodies
    uint32_t activeBodies = 0;     ///< Number of active (awake) bodies
    uint32_t sleepingBodies = 0;   ///< Number of sleeping bodies
    uint32_t staticBodies = 0;     ///< Number of static bodies
    uint32_t dynamicBodies = 0;    ///< Number of dynamic bodies
    uint32_t kinematicBodies = 0;  ///< Number of kinematic bodies

    // Contact and constraint counts
    uint32_t contactPointCount = 0;  ///< Number of contact points
    uint32_t constraintCount = 0;    ///< Number of constraints/joints
    uint32_t islandCount = 0;        ///< Number of simulation islands

    // Performance metrics (in milliseconds)
    float totalStepTime = 0.0f;    ///< Total physics step time
    float broadphaseTime = 0.0f;   ///< Broadphase collision detection time
    float narrowphaseTime = 0.0f;  ///< Narrowphase collision detection time
    float solverTime = 0.0f;       ///< Constraint solver time
    float integrationTime = 0.0f;  ///< Integration time
};

/// Configuration for physics simulation
struct PhysicsWorldConfig {
    math::Vec3 gravity = math::Vec3(0.0f, -9.81f, 0.0f);  ///< Gravity vector (m/s^2)
    float timeStep = 1.0f / 60.0f;                        ///< Fixed time step (seconds)
    uint32_t velocityIterations = 8;                      ///< Velocity solver iterations
    uint32_t positionIterations = 3;                      ///< Position solver iterations
    bool allowSleep = true;                               ///< Enable body sleeping
    float sleepLinearThreshold = 0.01f;                   ///< Linear velocity sleep threshold
    float sleepAngularThreshold = 0.01f;                  ///< Angular velocity sleep threshold
    float sleepTimeThreshold = 0.5f;                      ///< Time threshold for sleeping (seconds)
};

/// Physics debug panel for ImGui
///
/// This class provides a comprehensive debug interface for inspecting and controlling
/// the physics simulation. It displays:
/// - World statistics (body counts, contact counts, island counts)
/// - Simulation settings (gravity, time step, solver iterations)
/// - Visualization options (debug draw flags)
/// - Performance metrics (frame time breakdown)
///
/// The panel can both display read-only information and provide interactive controls
/// for modifying simulation parameters in real-time.
///
/// Thread safety:
/// - Must be called from the main thread (ImGui requirement)
/// - Does not own or modify physics world data directly
///
/// Example usage:
/// @code
/// PhysicsDebugPanel panel;
///
/// // In main loop
/// imgui.newFrame();
///
/// // Update stats from physics world
/// PhysicsWorldStats stats;
/// stats.totalBodies = world.getBodyCount();
/// stats.activeBodies = world.getActiveBodyCount();
/// // ... populate other stats
///
/// PhysicsWorldConfig config;
/// config.gravity = world.getGravity();
/// config.timeStep = world.getTimeStep();
/// // ... populate other config
///
/// // Render panel
/// if (panel.render(stats, config)) {
///     // Config was modified, apply changes to world
///     world.setGravity(config.gravity);
///     world.setTimeStep(config.timeStep);
/// }
///
/// imgui.render(commandBuffer);
/// @endcode
class PhysicsDebugPanel {
public:
    /// Create a physics debug panel
    PhysicsDebugPanel();

    /// Destructor
    ~PhysicsDebugPanel() = default;

    // Copyable and movable
    PhysicsDebugPanel(const PhysicsDebugPanel&) = default;
    PhysicsDebugPanel& operator=(const PhysicsDebugPanel&) = default;
    PhysicsDebugPanel(PhysicsDebugPanel&&) = default;
    PhysicsDebugPanel& operator=(PhysicsDebugPanel&&) = default;

    /// Render the physics debug panel
    ///
    /// This renders all panel sections including statistics, settings, and visualization options.
    /// The panel is rendered as an ImGui window.
    ///
    /// @param stats Current physics world statistics (read-only display)
    /// @param config Physics world configuration (can be modified by user)
    /// @return true if config was modified by user interaction, false otherwise
    bool render(const PhysicsWorldStats& stats, PhysicsWorldConfig& config);

    /// Render the physics debug panel with debug draw flags
    ///
    /// This variant also includes debug visualization controls.
    ///
    /// @param stats Current physics world statistics (read-only display)
    /// @param config Physics world configuration (can be modified by user)
    /// @param debugFlags Debug draw flags (can be modified by user)
    /// @return true if config or debugFlags were modified, false otherwise
    bool render(const PhysicsWorldStats& stats, PhysicsWorldConfig& config,
                debug::PhysicsDebugFlags& debugFlags);

    // === Window state ===

    /// Set whether the panel window is open
    /// @param open true to show window, false to hide
    void setOpen(bool open) noexcept { isOpen_ = open; }

    /// Get whether the panel window is open
    /// @return true if window is visible
    bool isOpen() const noexcept { return isOpen_; }

    /// Toggle the panel window open/closed state
    void toggleOpen() noexcept { isOpen_ = !isOpen_; }

    // === Panel sections ===

    /// Set whether to show the statistics section
    /// @param show true to show, false to hide
    void setShowStats(bool show) noexcept { showStats_ = show; }

    /// Get whether the statistics section is shown
    /// @return true if visible
    bool getShowStats() const noexcept { return showStats_; }

    /// Set whether to show the simulation settings section
    /// @param show true to show, false to hide
    void setShowSettings(bool show) noexcept { showSettings_ = show; }

    /// Get whether the simulation settings section is shown
    /// @return true if visible
    bool getShowSettings() const noexcept { return showSettings_; }

    /// Set whether to show the visualization options section
    /// @param show true to show, false to hide
    void setShowVisualization(bool show) noexcept { showVisualization_ = show; }

    /// Get whether the visualization options section is shown
    /// @return true if visible
    bool getShowVisualization() const noexcept { return showVisualization_; }

    /// Set whether to show the performance metrics section
    /// @param show true to show, false to hide
    void setShowPerformance(bool show) noexcept { showPerformance_ = show; }

    /// Get whether the performance metrics section is shown
    /// @return true if visible
    bool getShowPerformance() const noexcept { return showPerformance_; }

private:
    /// Render the statistics section
    /// @param stats World statistics
    void renderStatsSection(const PhysicsWorldStats& stats);

    /// Render the simulation settings section
    /// @param config World configuration (mutable)
    /// @return true if any setting was modified
    bool renderSettingsSection(PhysicsWorldConfig& config);

    /// Render the visualization options section
    /// @param flags Debug draw flags (mutable)
    /// @return true if flags were modified
    bool renderVisualizationSection(debug::PhysicsDebugFlags& flags);

    /// Render the performance metrics section
    /// @param stats World statistics (contains timing info)
    void renderPerformanceSection(const PhysicsWorldStats& stats);

    // Window state
    bool isOpen_ = true;  ///< Whether the panel window is visible

    // Section visibility
    bool showStats_ = true;          ///< Show statistics section
    bool showSettings_ = true;       ///< Show simulation settings section
    bool showVisualization_ = true;  ///< Show visualization options section
    bool showPerformance_ = true;    ///< Show performance metrics section
};

}  // namespace axiom::gui
