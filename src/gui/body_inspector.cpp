#include "axiom/gui/body_inspector.hpp"

#include "axiom/math/constants.hpp"
#include "axiom/math/utils.hpp"

#include <cmath>
#include <imgui.h>

namespace axiom::gui {

namespace {

// Helper function to convert quaternion to Euler angles (in degrees)
math::Vec3 quatToEuler(const math::Quat& q) {
    math::Vec3 euler;

    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    euler.x = std::atan2(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (std::abs(sinp) >= 1.0f) {
        euler.y = std::copysign(math::PI_F / 2.0f, sinp);  // Use 90 degrees if out of range
    } else {
        euler.y = std::asin(sinp);
    }

    // Yaw (z-axis rotation)
    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    euler.z = std::atan2(siny_cosp, cosy_cosp);

    // Convert to degrees
    euler.x = math::degrees(euler.x);
    euler.y = math::degrees(euler.y);
    euler.z = math::degrees(euler.z);

    return euler;
}

// Helper function to convert Euler angles (in degrees) to quaternion
math::Quat eulerToQuat(const math::Vec3& euler) {
    // Convert degrees to radians
    float roll = math::radians(euler.x);
    float pitch = math::radians(euler.y);
    float yaw = math::radians(euler.z);

    // Compute half angles
    float cr = std::cos(roll * 0.5f);
    float sr = std::sin(roll * 0.5f);
    float cp = std::cos(pitch * 0.5f);
    float sp = std::sin(pitch * 0.5f);
    float cy = std::cos(yaw * 0.5f);
    float sy = std::sin(yaw * 0.5f);

    math::Quat q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;

    return q;
}

// Helper to get body type name
const char* getBodyTypeName(BodyType type) {
    switch (type) {
    case BodyType::Static:
        return "Static";
    case BodyType::Dynamic:
        return "Dynamic";
    case BodyType::Kinematic:
        return "Kinematic";
    default:
        return "Unknown";
    }
}

// Helper to get shape type name
const char* getShapeTypeName(ShapeType type) {
    switch (type) {
    case ShapeType::Sphere:
        return "Sphere";
    case ShapeType::Box:
        return "Box";
    case ShapeType::Capsule:
        return "Capsule";
    case ShapeType::Cylinder:
        return "Cylinder";
    case ShapeType::Mesh:
        return "Mesh";
    case ShapeType::Convex:
        return "Convex";
    default:
        return "Unknown";
    }
}

}  // namespace

// Constructor
BodyInspector::BodyInspector() = default;

// Render the body inspector panel
bool BodyInspector::render(RigidBodyData& bodyData) {
    return render(bodyData, "Body Inspector");
}

// Render with custom window title
bool BodyInspector::render(RigidBodyData& bodyData, const char* windowTitle) {
    if (!isOpen_) {
        return false;
    }

    bool modified = false;

    ImGui::Begin(windowTitle, &isOpen_);

    // Identity section
    if (showIdentity_) {
        if (ImGui::CollapsingHeader("Identity", ImGuiTreeNodeFlags_DefaultOpen)) {
            modified |= renderIdentitySection(bodyData);
        }
    }

    // Transform section
    if (showTransform_) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            modified |= renderTransformSection(bodyData);
        }
    }

    // Dynamics section
    if (showDynamics_) {
        if (ImGui::CollapsingHeader("Dynamics", ImGuiTreeNodeFlags_DefaultOpen)) {
            modified |= renderDynamicsSection(bodyData);
        }
    }

    // Mass properties section
    if (showMassProperties_) {
        if (ImGui::CollapsingHeader("Mass Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            modified |= renderMassPropertiesSection(bodyData);
        }
    }

    // Shape section
    if (showShape_) {
        if (ImGui::CollapsingHeader("Shape", ImGuiTreeNodeFlags_DefaultOpen)) {
            modified |= renderShapeSection(bodyData);
        }
    }

    // Material section
    if (showMaterial_) {
        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
            modified |= renderMaterialSection(bodyData);
        }
    }

    // Filtering section
    if (showFiltering_) {
        if (ImGui::CollapsingHeader("Collision Filtering")) {
            modified |= renderFilteringSection(bodyData);
        }
    }

    // Sleep section
    if (showSleep_) {
        if (ImGui::CollapsingHeader("Sleep State")) {
            modified |= renderSleepSection(bodyData);
        }
    }

    ImGui::End();

    return modified;
}

// Render identity section
bool BodyInspector::renderIdentitySection(RigidBodyData& bodyData) {
    bool modified = false;

    ImGui::Indent();

    // ID (read-only)
    ImGui::Text("ID: %u", bodyData.id);

    // Type
    int currentType = static_cast<int>(bodyData.type);
    const char* typeNames[] = {"Static", "Dynamic", "Kinematic"};
    if (ImGui::Combo("Type", &currentType, typeNames, 3)) {
        bodyData.type = static_cast<BodyType>(currentType);
        modified = true;
    }

    // Name (editable text)
    char nameBuffer[256];
    std::strncpy(nameBuffer, bodyData.name.c_str(), sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';

    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
        bodyData.name = nameBuffer;
        modified = true;
    }

    ImGui::Unindent();

    return modified;
}

// Render transform section
bool BodyInspector::renderTransformSection(RigidBodyData& bodyData) {
    bool modified = false;

    ImGui::Indent();

    // Position
    ImGui::Text("Position (m):");
    ImGui::Indent();
    modified |= ImGui::DragFloat("X##pos", &bodyData.position.x, 0.1f, -1000.0f, 1000.0f, "%.3f");
    modified |= ImGui::DragFloat("Y##pos", &bodyData.position.y, 0.1f, -1000.0f, 1000.0f, "%.3f");
    modified |= ImGui::DragFloat("Z##pos", &bodyData.position.z, 0.1f, -1000.0f, 1000.0f, "%.3f");
    ImGui::Unindent();

    ImGui::Spacing();

    // Rotation display mode toggle
    ImGui::Checkbox("Use Euler Angles", &useEulerAngles_);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle between Euler angles (degrees) and Quaternion representation");
    }

    ImGui::Spacing();

    // Rotation
    if (useEulerAngles_) {
        // Display as Euler angles
        ImGui::Text("Rotation (degrees):");
        ImGui::Indent();

        math::Vec3 euler = quatToEuler(bodyData.rotation);
        bool rotationModified = false;

        rotationModified |= ImGui::DragFloat("Pitch (X)##rot", &euler.x, 1.0f, -180.0f, 180.0f,
                                             "%.1f");
        rotationModified |= ImGui::DragFloat("Yaw (Y)##rot", &euler.y, 1.0f, -180.0f, 180.0f,
                                             "%.1f");
        rotationModified |= ImGui::DragFloat("Roll (Z)##rot", &euler.z, 1.0f, -180.0f, 180.0f,
                                             "%.1f");

        if (rotationModified) {
            bodyData.rotation = eulerToQuat(euler);
            modified = true;
        }

        ImGui::Unindent();
    } else {
        // Display as quaternion
        ImGui::Text("Rotation (Quaternion):");
        ImGui::Indent();

        modified |= ImGui::DragFloat("X##quat", &bodyData.rotation.x, 0.01f, -1.0f, 1.0f, "%.3f");
        modified |= ImGui::DragFloat("Y##quat", &bodyData.rotation.y, 0.01f, -1.0f, 1.0f, "%.3f");
        modified |= ImGui::DragFloat("Z##quat", &bodyData.rotation.z, 0.01f, -1.0f, 1.0f, "%.3f");
        modified |= ImGui::DragFloat("W##quat", &bodyData.rotation.w, 0.01f, -1.0f, 1.0f, "%.3f");

        ImGui::Unindent();

        // Show normalization warning if quaternion is not normalized
        float length = std::sqrt(
            bodyData.rotation.x * bodyData.rotation.x + bodyData.rotation.y * bodyData.rotation.y +
            bodyData.rotation.z * bodyData.rotation.z + bodyData.rotation.w * bodyData.rotation.w);

        if (std::abs(length - 1.0f) > 0.01f) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                               "Warning: Quaternion not normalized!");
            ImGui::SameLine();
            if (ImGui::SmallButton("Normalize")) {
                if (length > 0.0001f) {
                    bodyData.rotation.x /= length;
                    bodyData.rotation.y /= length;
                    bodyData.rotation.z /= length;
                    bodyData.rotation.w /= length;
                    modified = true;
                }
            }
        }
    }

    ImGui::Unindent();

    return modified;
}

// Render dynamics section
bool BodyInspector::renderDynamicsSection(RigidBodyData& bodyData) {
    bool modified = false;

    ImGui::Indent();

    // Linear velocity
    ImGui::Text("Linear Velocity (m/s):");
    ImGui::Indent();
    modified |= ImGui::DragFloat("X##linvel", &bodyData.linearVelocity.x, 0.1f, -100.0f, 100.0f,
                                 "%.2f");
    modified |= ImGui::DragFloat("Y##linvel", &bodyData.linearVelocity.y, 0.1f, -100.0f, 100.0f,
                                 "%.2f");
    modified |= ImGui::DragFloat("Z##linvel", &bodyData.linearVelocity.z, 0.1f, -100.0f, 100.0f,
                                 "%.2f");
    ImGui::Unindent();

    // Display magnitude
    float linSpeed = std::sqrt(bodyData.linearVelocity.x * bodyData.linearVelocity.x +
                               bodyData.linearVelocity.y * bodyData.linearVelocity.y +
                               bodyData.linearVelocity.z * bodyData.linearVelocity.z);
    ImGui::Text("  Speed: %.2f m/s", static_cast<double>(linSpeed));

    ImGui::Spacing();

    // Angular velocity
    ImGui::Text("Angular Velocity (rad/s):");
    ImGui::Indent();
    modified |= ImGui::DragFloat("X##angvel", &bodyData.angularVelocity.x, 0.1f, -50.0f, 50.0f,
                                 "%.2f");
    modified |= ImGui::DragFloat("Y##angvel", &bodyData.angularVelocity.y, 0.1f, -50.0f, 50.0f,
                                 "%.2f");
    modified |= ImGui::DragFloat("Z##angvel", &bodyData.angularVelocity.z, 0.1f, -50.0f, 50.0f,
                                 "%.2f");
    ImGui::Unindent();

    // Display magnitude
    float angSpeed = std::sqrt(bodyData.angularVelocity.x * bodyData.angularVelocity.x +
                               bodyData.angularVelocity.y * bodyData.angularVelocity.y +
                               bodyData.angularVelocity.z * bodyData.angularVelocity.z);
    ImGui::Text("  Magnitude: %.2f rad/s", static_cast<double>(angSpeed));

    // Quick zero buttons
    ImGui::Spacing();
    if (ImGui::Button("Zero Linear Velocity")) {
        bodyData.linearVelocity = math::Vec3(0.0f, 0.0f, 0.0f);
        modified = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Zero Angular Velocity")) {
        bodyData.angularVelocity = math::Vec3(0.0f, 0.0f, 0.0f);
        modified = true;
    }

    ImGui::Unindent();

    return modified;
}

// Render mass properties section
bool BodyInspector::renderMassPropertiesSection(RigidBodyData& bodyData) {
    bool modified = false;

    ImGui::Indent();

    // Mass
    modified |= ImGui::DragFloat("Mass (kg)", &bodyData.mass, 0.1f, 0.001f, 10000.0f, "%.3f");

    // Show infinite mass for static/kinematic bodies
    if (bodyData.type != BodyType::Dynamic) {
        ImGui::SameLine();
        ImGui::TextDisabled("(Infinite)");
    }

    ImGui::Spacing();

    // Inertia tensor (diagonal only for simplicity)
    ImGui::Text("Inertia Tensor (diagonal):");
    ImGui::Indent();
    modified |= ImGui::DragFloat("Ixx##inertia", &bodyData.inertiaTensor.x, 0.1f, 0.001f, 10000.0f,
                                 "%.3f");
    modified |= ImGui::DragFloat("Iyy##inertia", &bodyData.inertiaTensor.y, 0.1f, 0.001f, 10000.0f,
                                 "%.3f");
    modified |= ImGui::DragFloat("Izz##inertia", &bodyData.inertiaTensor.z, 0.1f, 0.001f, 10000.0f,
                                 "%.3f");
    ImGui::Unindent();

    ImGui::Spacing();

    // Damping
    ImGui::Text("Damping:");
    ImGui::Indent();
    modified |= ImGui::SliderFloat("Linear##damp", &bodyData.linearDamping, 0.0f, 1.0f, "%.3f");
    modified |= ImGui::SliderFloat("Angular##damp", &bodyData.angularDamping, 0.0f, 1.0f, "%.3f");
    ImGui::Unindent();

    ImGui::Unindent();

    return modified;
}

// Render shape section
bool BodyInspector::renderShapeSection(RigidBodyData& bodyData) {
    bool modified = false;

    ImGui::Indent();

    // Shape type
    int currentShape = static_cast<int>(bodyData.shapeType);
    const char* shapeNames[] = {"Sphere", "Box", "Capsule", "Cylinder", "Mesh", "Convex"};
    if (ImGui::Combo("Shape Type", &currentShape, shapeNames, 6)) {
        bodyData.shapeType = static_cast<ShapeType>(currentShape);
        modified = true;
    }

    ImGui::Spacing();

    // Shape-specific parameters
    ImGui::Text("Dimensions:");
    ImGui::Indent();

    switch (bodyData.shapeType) {
    case ShapeType::Sphere:
        modified |= ImGui::DragFloat("Radius", &bodyData.shapeExtents.x, 0.1f, 0.01f, 100.0f,
                                     "%.2f");
        break;

    case ShapeType::Box:
        modified |= ImGui::DragFloat("Width (X)", &bodyData.shapeExtents.x, 0.1f, 0.01f, 100.0f,
                                     "%.2f");
        modified |= ImGui::DragFloat("Height (Y)", &bodyData.shapeExtents.y, 0.1f, 0.01f, 100.0f,
                                     "%.2f");
        modified |= ImGui::DragFloat("Depth (Z)", &bodyData.shapeExtents.z, 0.1f, 0.01f, 100.0f,
                                     "%.2f");
        break;

    case ShapeType::Capsule:
        modified |= ImGui::DragFloat("Radius", &bodyData.shapeExtents.x, 0.1f, 0.01f, 100.0f,
                                     "%.2f");
        modified |= ImGui::DragFloat("Height", &bodyData.shapeExtents.y, 0.1f, 0.01f, 100.0f,
                                     "%.2f");
        break;

    case ShapeType::Cylinder:
        modified |= ImGui::DragFloat("Radius", &bodyData.shapeExtents.x, 0.1f, 0.01f, 100.0f,
                                     "%.2f");
        modified |= ImGui::DragFloat("Height", &bodyData.shapeExtents.y, 0.1f, 0.01f, 100.0f,
                                     "%.2f");
        break;

    case ShapeType::Mesh:
    case ShapeType::Convex:
        ImGui::TextDisabled("Mesh-specific parameters not editable");
        break;
    }

    ImGui::Unindent();
    ImGui::Unindent();

    return modified;
}

// Render material section
bool BodyInspector::renderMaterialSection(RigidBodyData& bodyData) {
    bool modified = false;

    ImGui::Indent();

    modified |= ImGui::SliderFloat("Restitution", &bodyData.material.restitution, 0.0f, 1.0f,
                                   "%.2f");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Bounciness: 0 = no bounce, 1 = perfect bounce");
    }

    modified |= ImGui::SliderFloat("Friction", &bodyData.material.friction, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Surface friction coefficient");
    }

    modified |= ImGui::SliderFloat("Rolling Friction", &bodyData.material.rollingFriction, 0.0f,
                                   1.0f, "%.3f");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Resistance to rolling motion");
    }

    modified |= ImGui::DragFloat("Density (kg/m^3)", &bodyData.material.density, 10.0f, 0.1f,
                                 20000.0f, "%.1f");

    // Material presets
    ImGui::Spacing();
    ImGui::Text("Presets:");
    if (ImGui::SmallButton("Wood")) {
        bodyData.material.density = 700.0f;
        bodyData.material.restitution = 0.4f;
        bodyData.material.friction = 0.6f;
        modified = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Steel")) {
        bodyData.material.density = 7850.0f;
        bodyData.material.restitution = 0.5f;
        bodyData.material.friction = 0.7f;
        modified = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Rubber")) {
        bodyData.material.density = 1100.0f;
        bodyData.material.restitution = 0.8f;
        bodyData.material.friction = 0.9f;
        modified = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Ice")) {
        bodyData.material.density = 920.0f;
        bodyData.material.restitution = 0.1f;
        bodyData.material.friction = 0.02f;
        modified = true;
    }

    ImGui::Unindent();

    return modified;
}

// Render filtering section
bool BodyInspector::renderFilteringSection(RigidBodyData& bodyData) {
    bool modified = false;

    ImGui::Indent();

    // Category bits (display as hex)
    int categoryBits = static_cast<int>(bodyData.filter.categoryBits);
    if (ImGui::InputInt("Category Bits", &categoryBits, 1, 16,
                        ImGuiInputTextFlags_CharsHexadecimal)) {
        bodyData.filter.categoryBits = static_cast<uint32_t>(categoryBits);
        modified = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Collision category this body belongs to (bitmask)");
    }

    // Mask bits (display as hex)
    int maskBits = static_cast<int>(bodyData.filter.maskBits);
    if (ImGui::InputInt("Mask Bits", &maskBits, 1, 16, ImGuiInputTextFlags_CharsHexadecimal)) {
        bodyData.filter.maskBits = static_cast<uint32_t>(maskBits);
        modified = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Categories this body collides with (bitmask)");
    }

    // Group index
    int groupIndex = static_cast<int>(bodyData.filter.groupIndex);
    if (ImGui::InputInt("Group Index", &groupIndex, 1, 10)) {
        bodyData.filter.groupIndex = static_cast<int16_t>(groupIndex);
        modified = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Negative = never collide, positive = always collide (same group)");
    }

    ImGui::Unindent();

    return modified;
}

// Render sleep section
bool BodyInspector::renderSleepSection(RigidBodyData& bodyData) {
    bool modified = false;

    ImGui::Indent();

    // Sleep status (read-only indicator)
    if (bodyData.sleep.isSleeping) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Status: SLEEPING");
    } else {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: AWAKE");
    }

    // Sleep time (read-only)
    ImGui::Text("Sleep Time: %.2f s", static_cast<double>(bodyData.sleep.sleepTime));

    ImGui::Spacing();

    // Allow sleep toggle
    modified |= ImGui::Checkbox("Allow Sleep", &bodyData.sleep.allowSleep);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Allow this body to go to sleep when inactive");
    }

    if (bodyData.sleep.allowSleep) {
        ImGui::Spacing();
        ImGui::Text("Thresholds:");
        ImGui::Indent();

        modified |= ImGui::DragFloat("Linear Threshold", &bodyData.sleep.linearThreshold, 0.001f,
                                     0.0f, 1.0f, "%.4f");
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Linear velocity threshold for sleeping (m/s)");
        }

        modified |= ImGui::DragFloat("Angular Threshold", &bodyData.sleep.angularThreshold, 0.001f,
                                     0.0f, 1.0f, "%.4f");
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Angular velocity threshold for sleeping (rad/s)");
        }

        ImGui::Unindent();
    }

    // Wake up button
    if (bodyData.sleep.isSleeping) {
        ImGui::Spacing();
        if (ImGui::Button("Wake Up")) {
            bodyData.sleep.isSleeping = false;
            bodyData.sleep.sleepTime = 0.0f;
            modified = true;
        }
    }

    ImGui::Unindent();

    return modified;
}

}  // namespace axiom::gui
