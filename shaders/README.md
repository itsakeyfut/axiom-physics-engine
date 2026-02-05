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
