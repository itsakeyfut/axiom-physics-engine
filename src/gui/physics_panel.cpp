#include "axiom/gui/physics_panel.hpp"

#include <imgui.h>

namespace axiom::gui {

namespace {

// Helper to check if a flag bit is set
bool hasDebugFlag(debug::PhysicsDebugFlags flags, debug::PhysicsDebugFlags flag) {
    return (flags & flag) != debug::PhysicsDebugFlags::None;
}

// Helper to toggle a flag bit
void toggleDebugFlag(debug::PhysicsDebugFlags& flags, debug::PhysicsDebugFlags flag) {
    if (hasDebugFlag(flags, flag)) {
        flags = static_cast<debug::PhysicsDebugFlags>(static_cast<uint32_t>(flags) &
                                                      ~static_cast<uint32_t>(flag));
    } else {
        flags = flags | flag;
    }
}

}  // namespace

// Constructor
PhysicsDebugPanel::PhysicsDebugPanel() = default;

// Render the physics debug panel
bool PhysicsDebugPanel::render(const PhysicsWorldStats& stats, PhysicsWorldConfig& config) {
    if (!isOpen_) {
        return false;
    }

    bool modified = false;

    ImGui::Begin("Physics Debug Panel", &isOpen_);

    // Statistics section
    if (showStats_) {
        if (ImGui::CollapsingHeader("World Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderStatsSection(stats);
        }
    }

    // Simulation settings section
    if (showSettings_) {
        if (ImGui::CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            modified |= renderSettingsSection(config);
        }
    }

    // Performance metrics section
    if (showPerformance_) {
        if (ImGui::CollapsingHeader("Performance Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderPerformanceSection(stats);
        }
    }

    ImGui::End();

    return modified;
}

// Render with debug draw flags
bool PhysicsDebugPanel::render(const PhysicsWorldStats& stats, PhysicsWorldConfig& config,
                               debug::PhysicsDebugFlags& debugFlags) {
    if (!isOpen_) {
        return false;
    }

    bool modified = false;

    ImGui::Begin("Physics Debug Panel", &isOpen_);

    // Statistics section
    if (showStats_) {
        if (ImGui::CollapsingHeader("World Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderStatsSection(stats);
        }
    }

    // Simulation settings section
    if (showSettings_) {
        if (ImGui::CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            modified |= renderSettingsSection(config);
        }
    }

    // Visualization options section
    if (showVisualization_) {
        if (ImGui::CollapsingHeader("Visualization Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            modified |= renderVisualizationSection(debugFlags);
        }
    }

    // Performance metrics section
    if (showPerformance_) {
        if (ImGui::CollapsingHeader("Performance Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderPerformanceSection(stats);
        }
    }

    ImGui::End();

    return modified;
}

// Render statistics section
void PhysicsDebugPanel::renderStatsSection(const PhysicsWorldStats& stats) {
    ImGui::Indent();

    // Body counts
    ImGui::Text("Bodies:");
    ImGui::Indent();
    ImGui::Text("Total: %u", stats.totalBodies);
    ImGui::Text("Active: %u", stats.activeBodies);
    ImGui::Text("Sleeping: %u", stats.sleepingBodies);
    ImGui::Separator();
    ImGui::Text("Static: %u", stats.staticBodies);
    ImGui::Text("Dynamic: %u", stats.dynamicBodies);
    ImGui::Text("Kinematic: %u", stats.kinematicBodies);
    ImGui::Unindent();

    ImGui::Spacing();

    // Contact and constraint counts
    ImGui::Text("Contacts: %u", stats.contactPointCount);
    ImGui::Text("Constraints: %u", stats.constraintCount);
    ImGui::Text("Islands: %u", stats.islandCount);

    ImGui::Unindent();
}

// Render simulation settings section
bool PhysicsDebugPanel::renderSettingsSection(PhysicsWorldConfig& config) {
    bool modified = false;

    ImGui::Indent();

    // Gravity
    ImGui::Text("Gravity (m/s^2):");
    ImGui::Indent();
    modified |= ImGui::DragFloat("X##gravity", &config.gravity.x, 0.1f, -50.0f, 50.0f, "%.2f");
    modified |= ImGui::DragFloat("Y##gravity", &config.gravity.y, 0.1f, -50.0f, 50.0f, "%.2f");
    modified |= ImGui::DragFloat("Z##gravity", &config.gravity.z, 0.1f, -50.0f, 50.0f, "%.2f");
    ImGui::Unindent();

    // Quick gravity presets
    ImGui::Text("Presets:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Earth")) {
        config.gravity = math::Vec3(0.0f, -9.81f, 0.0f);
        modified = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Moon")) {
        config.gravity = math::Vec3(0.0f, -1.62f, 0.0f);
        modified = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Zero-G")) {
        config.gravity = math::Vec3(0.0f, 0.0f, 0.0f);
        modified = true;
    }

    ImGui::Spacing();

    // Time step
    float timeStepMs = config.timeStep * 1000.0f;
    if (ImGui::DragFloat("Time Step (ms)", &timeStepMs, 0.1f, 1.0f, 100.0f, "%.2f")) {
        config.timeStep = timeStepMs / 1000.0f;
        modified = true;
    }

    // Display equivalent FPS
    float fps = 1.0f / config.timeStep;
    ImGui::Text("(%.1f FPS)", static_cast<double>(fps));

    ImGui::Spacing();

    // Solver iterations
    int velocityIter = static_cast<int>(config.velocityIterations);
    if (ImGui::SliderInt("Velocity Iterations", &velocityIter, 1, 20)) {
        config.velocityIterations = static_cast<uint32_t>(velocityIter);
        modified = true;
    }

    int positionIter = static_cast<int>(config.positionIterations);
    if (ImGui::SliderInt("Position Iterations", &positionIter, 1, 20)) {
        config.positionIterations = static_cast<uint32_t>(positionIter);
        modified = true;
    }

    ImGui::Spacing();

    // Sleep settings
    modified |= ImGui::Checkbox("Allow Sleep", &config.allowSleep);

    if (config.allowSleep) {
        ImGui::Indent();
        modified |= ImGui::DragFloat("Linear Threshold", &config.sleepLinearThreshold, 0.001f, 0.0f,
                                     1.0f, "%.4f");
        modified |= ImGui::DragFloat("Angular Threshold", &config.sleepAngularThreshold, 0.001f,
                                     0.0f, 1.0f, "%.4f");
        modified |= ImGui::DragFloat("Time Threshold (s)", &config.sleepTimeThreshold, 0.01f, 0.0f,
                                     5.0f, "%.2f");
        ImGui::Unindent();
    }

    ImGui::Unindent();

    return modified;
}

// Render visualization options section
bool PhysicsDebugPanel::renderVisualizationSection(debug::PhysicsDebugFlags& flags) {
    bool modified = false;

    ImGui::Indent();

    // Quick toggle all
    bool allEnabled = (flags == debug::PhysicsDebugFlags::All);
    if (ImGui::Checkbox("Enable All", &allEnabled)) {
        flags = allEnabled ? debug::PhysicsDebugFlags::All : debug::PhysicsDebugFlags::None;
        modified = true;
    }

    ImGui::Separator();

    // Individual flags
    bool shapesEnabled = hasDebugFlag(flags, debug::PhysicsDebugFlags::Shapes);
    if (ImGui::Checkbox("Shapes", &shapesEnabled)) {
        toggleDebugFlag(flags, debug::PhysicsDebugFlags::Shapes);
        modified = true;
    }

    bool aabbsEnabled = hasDebugFlag(flags, debug::PhysicsDebugFlags::AABBs);
    if (ImGui::Checkbox("AABBs", &aabbsEnabled)) {
        toggleDebugFlag(flags, debug::PhysicsDebugFlags::AABBs);
        modified = true;
    }

    bool contactsEnabled = hasDebugFlag(flags, debug::PhysicsDebugFlags::Contacts);
    if (ImGui::Checkbox("Contacts", &contactsEnabled)) {
        toggleDebugFlag(flags, debug::PhysicsDebugFlags::Contacts);
        modified = true;
    }

    bool constraintsEnabled = hasDebugFlag(flags, debug::PhysicsDebugFlags::Constraints);
    if (ImGui::Checkbox("Constraints", &constraintsEnabled)) {
        toggleDebugFlag(flags, debug::PhysicsDebugFlags::Constraints);
        modified = true;
    }

    bool velocitiesEnabled = hasDebugFlag(flags, debug::PhysicsDebugFlags::Velocities);
    if (ImGui::Checkbox("Velocities", &velocitiesEnabled)) {
        toggleDebugFlag(flags, debug::PhysicsDebugFlags::Velocities);
        modified = true;
    }

    bool angularVelocitiesEnabled = hasDebugFlag(flags,
                                                 debug::PhysicsDebugFlags::AngularVelocities);
    if (ImGui::Checkbox("Angular Velocities", &angularVelocitiesEnabled)) {
        toggleDebugFlag(flags, debug::PhysicsDebugFlags::AngularVelocities);
        modified = true;
    }

    bool forcesEnabled = hasDebugFlag(flags, debug::PhysicsDebugFlags::Forces);
    if (ImGui::Checkbox("Forces", &forcesEnabled)) {
        toggleDebugFlag(flags, debug::PhysicsDebugFlags::Forces);
        modified = true;
    }

    bool islandsEnabled = hasDebugFlag(flags, debug::PhysicsDebugFlags::Islands);
    if (ImGui::Checkbox("Islands", &islandsEnabled)) {
        toggleDebugFlag(flags, debug::PhysicsDebugFlags::Islands);
        modified = true;
    }

    bool centerOfMassEnabled = hasDebugFlag(flags, debug::PhysicsDebugFlags::CenterOfMass);
    if (ImGui::Checkbox("Center of Mass", &centerOfMassEnabled)) {
        toggleDebugFlag(flags, debug::PhysicsDebugFlags::CenterOfMass);
        modified = true;
    }

    bool localAxesEnabled = hasDebugFlag(flags, debug::PhysicsDebugFlags::LocalAxes);
    if (ImGui::Checkbox("Local Axes", &localAxesEnabled)) {
        toggleDebugFlag(flags, debug::PhysicsDebugFlags::LocalAxes);
        modified = true;
    }

    ImGui::Unindent();

    return modified;
}

// Render performance metrics section
void PhysicsDebugPanel::renderPerformanceSection(const PhysicsWorldStats& stats) {
    ImGui::Indent();

    // Total step time
    ImGui::Text("Total Step Time: %.2f ms", static_cast<double>(stats.totalStepTime));

    // Show FPS
    float fps = (stats.totalStepTime > 0.0f) ? (1000.0f / stats.totalStepTime) : 0.0f;
    ImGui::Text("Physics FPS: %.1f", static_cast<double>(fps));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Breakdown:");

    // Time breakdown
    ImGui::Indent();
    ImGui::Text("Broadphase:   %.2f ms", static_cast<double>(stats.broadphaseTime));
    ImGui::Text("Narrowphase:  %.2f ms", static_cast<double>(stats.narrowphaseTime));
    ImGui::Text("Solver:       %.2f ms", static_cast<double>(stats.solverTime));
    ImGui::Text("Integration:  %.2f ms", static_cast<double>(stats.integrationTime));
    ImGui::Unindent();

    ImGui::Spacing();

    // Visual breakdown with progress bars
    if (stats.totalStepTime > 0.0f) {
        ImGui::Separator();
        ImGui::Text("Time Distribution:");

        float broadphasePercent = (stats.broadphaseTime / stats.totalStepTime);
        float narrowphasePercent = (stats.narrowphaseTime / stats.totalStepTime);
        float solverPercent = (stats.solverTime / stats.totalStepTime);
        float integrationPercent = (stats.integrationTime / stats.totalStepTime);

        ImGui::ProgressBar(broadphasePercent, ImVec2(-1, 0), "");
        ImGui::SameLine(0, 5);
        ImGui::Text("Broadphase (%.1f%%)", static_cast<double>(broadphasePercent * 100.0f));

        ImGui::ProgressBar(narrowphasePercent, ImVec2(-1, 0), "");
        ImGui::SameLine(0, 5);
        ImGui::Text("Narrowphase (%.1f%%)", static_cast<double>(narrowphasePercent * 100.0f));

        ImGui::ProgressBar(solverPercent, ImVec2(-1, 0), "");
        ImGui::SameLine(0, 5);
        ImGui::Text("Solver (%.1f%%)", static_cast<double>(solverPercent * 100.0f));

        ImGui::ProgressBar(integrationPercent, ImVec2(-1, 0), "");
        ImGui::SameLine(0, 5);
        ImGui::Text("Integration (%.1f%%)", static_cast<double>(integrationPercent * 100.0f));
    }

    ImGui::Unindent();
}

}  // namespace axiom::gui
