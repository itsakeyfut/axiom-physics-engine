// Simple compute shader for testing shader module loading (Slang)
// This shader adds two arrays element-wise

// Input buffers
[[vk::binding(0, 0)]]
StructuredBuffer<float> inputA;

[[vk::binding(1, 0)]]
StructuredBuffer<float> inputB;

// Output buffer
[[vk::binding(2, 0)]]
RWStructuredBuffer<float> output;

[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint idx = dispatchThreadID.x;
    output[idx] = inputA[idx] + inputB[idx];
}
