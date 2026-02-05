# Shaders Directory

This directory contains Slang/HLSL shaders used by the Axiom Physics Engine.

## Shader Language

We use **Slang** as our primary shader language. Slang is a superset of HLSL that provides:
- Modern shader language features
- Better cross-platform support
- Enhanced type safety
- Improved debugging capabilities

## Structure

- `test/` - Test shaders for unit testing
- Future: Additional subdirectories for production shaders

## Compilation

Shaders must be compiled to SPIR-V format before use. Use `slangc`:

```bash
# Compile Slang shader (entry point specified in code)
slangc -target spirv -entry main shader.slang -o shader.spv

# Or specify stage explicitly if needed
slangc -target spirv -stage compute -entry main shader.slang -o shader.spv
```

**Note:** Slang uses the `.slang` extension regardless of shader stage (compute, vertex, fragment, etc.). The shader stage is determined by the entry point attributes in the code (e.g., `[shader("compute")]` or `[numthreads(...)]`).

## Test Shaders

### simple.slang
A simple compute shader that adds two arrays element-wise. Used for testing shader module loading and basic compute pipeline functionality.

**Language:** Slang

**Bindings:**
- Binding 0, Set 0: Input buffer A (StructuredBuffer<float>)
- Binding 1, Set 0: Input buffer B (StructuredBuffer<float>)
- Binding 2, Set 0: Output buffer (RWStructuredBuffer<float>)

**Workgroup size:** 256 threads per group

### array_add.slang
Comprehensive compute shader for array addition with bounds checking. Performs element-wise addition: `output[i] = inputA[i] + inputB[i]`. This shader is used for integration testing of the complete Vulkan compute pipeline infrastructure.

**Language:** Slang

**Bindings:**
- Binding 0, Set 0: Input buffer A (StructuredBuffer<float>, read-only)
- Binding 1, Set 0: Input buffer B (StructuredBuffer<float>, read-only)
- Binding 2, Set 0: Output buffer (RWStructuredBuffer<float>, write-only)

**Push Constants:**
- `count` (uint32_t): Number of elements to process (for bounds checking)

**Workgroup size:** 256 threads per group (1D)

**Compilation:**
```bash
# Compile array_add.slang to SPIR-V
slangc -target spirv -entry main shaders/test/array_add.slang -o shaders/test/array_add.comp.spv

# Verify compilation
spirv-val shaders/test/array_add.comp.spv  # Optional: validate SPIR-V output
```

**Usage in tests:**
This shader is used by `tests/gpu/test_compute.cpp` to verify:
- GPU buffer upload/download functionality
- Descriptor set binding and updates
- Push constant handling
- Compute shader dispatch with various array sizes
- Floating point precision on GPU vs CPU calculations
