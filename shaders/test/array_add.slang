// Array addition compute shader for testing Vulkan compute infrastructure
// This shader performs element-wise addition of two input arrays: output[i] = inputA[i] + inputB[i]

// Input buffer A (read-only)
[[vk::binding(0, 0)]]
StructuredBuffer<float> inputA;

// Input buffer B (read-only)
[[vk::binding(1, 0)]]
StructuredBuffer<float> inputB;

// Output buffer (write-only)
[[vk::binding(2, 0)]]
RWStructuredBuffer<float> output;

// Push constants for array size
[[vk::push_constant]]
struct PushConstants {
    uint count;  // Number of elements to process
};

[[vk::push_constant]]
PushConstants pc;

// Compute shader entry point with 256 threads per workgroup
[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint idx = dispatchThreadID.x;

    // Bounds check to prevent out-of-bounds access
    if (idx < pc.count) {
        output[idx] = inputA[idx] + inputB[idx];
    }
}
