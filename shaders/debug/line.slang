// Debug line rendering shaders for Axiom Physics Engine
// This file contains both vertex and fragment shaders for debug primitive rendering

// Push constants for view-projection matrix
struct PushConstants {
    float4x4 viewProj;
};

[[vk::push_constant]]
PushConstants pc;

// Vertex input structure
struct VertexInput {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float4 color : COLOR;
};

// Vertex output / Fragment input structure
struct VertexOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float4 color : COLOR;
};

// Fragment output structure
struct FragmentOutput {
    [[vk::location(0)]] float4 color : SV_Target0;
};

// ============================================================================
// Vertex Shader
// ============================================================================

[shader("vertex")]
VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;

    // Transform vertex position by view-projection matrix
    output.position = mul(pc.viewProj, float4(input.position, 1.0));

    // Pass color to fragment shader
    output.color = input.color;

    return output;
}

// ============================================================================
// Fragment Shader
// ============================================================================

[shader("fragment")]
FragmentOutput fragmentMain(VertexOutput input) {
    FragmentOutput output;

    // Output the interpolated color
    output.color = input.color;

    return output;
}
