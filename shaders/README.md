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

Shaders must be compiled to SPIR-V format before use. Use `slangc` from the Vulkan SDK:

```bash
# Compile compute shader
slangc -target spirv -stage compute shader.comp -o shader.comp.spv

# Compile vertex shader
slangc -target spirv -stage vertex shader.vert -o shader.vert.spv

# Compile fragment shader
slangc -target spirv -stage fragment shader.frag -o shader.frag.spv
```

### Alternative: GLSL Support

While we primarily use Slang, GLSL shaders can also be compiled using `glslc`:

```bash
glslc -fshader-stage=compute shader.comp -o shader.comp.spv
```

## Test Shaders

### simple.comp
A simple compute shader (Slang/HLSL) that adds two arrays element-wise. Used for testing shader module loading and basic compute pipeline functionality.

**Language:** Slang/HLSL

**Bindings:**
- Binding 0, Set 0: Input buffer A (StructuredBuffer<float>)
- Binding 1, Set 0: Input buffer B (StructuredBuffer<float>)
- Binding 2, Set 0: Output buffer (RWStructuredBuffer<float>)

**Workgroup size:** 256 threads per group

### array_add.comp
Array addition compute shader (GLSL) for comprehensive compute pipeline testing. Verifies GPU compute infrastructure by performing element-wise addition with CPU result validation.

**Language:** GLSL

**Bindings:**
- Binding 0: Input buffer A (readonly storage buffer)
- Binding 1: Input buffer B (readonly storage buffer)
- Binding 2: Output buffer (writeonly storage buffer)

**Push constants:**
- uint count: Number of elements to process

**Workgroup size:** 256 threads per group

**Features:**
- Bounds checking to handle non-aligned buffer sizes
- Push constant for dynamic element count
- Used by test_compute.cpp for integration testing
