## Summary

<!-- Concise description of what this PR does (1-2 sentences) -->

## Motivation

<!-- Why is this change needed? What problem does it solve? -->

## Related Issues

Closes #

## Changes Made

<!-- Detailed breakdown of the changes -->

### Implementation Details

<!-- Key technical decisions, algorithms, or design patterns used -->

### Root Cause (for bug fixes)

<!-- What was causing the issue and where? -->

## Trade-offs

<!-- Pros, cons, and any technical debt introduced -->

## **Pros:**

## **Cons:**

## Testing

<!-- Describe how you tested these changes -->

```bash
# Build and run tests
cmake --preset windows-debug
cmake --build build/windows-debug
ctest --preset windows-debug

# Run specific tests
./build/windows-debug/bin/axiom_tests --gtest_filter="TestSuite.*"

# Run benchmarks (if applicable)
cmake --preset windows-release
cmake --build build/windows-release
./build/windows-release/bin/axiom_benchmark
```

**Test coverage:**

- [ ] Unit tests added/updated
- [ ] Manual testing completed
- [ ] Edge cases covered

**Expected behavior:**

<!-- What should happen with these changes? -->

## Performance Impact

- [ ] No performance impact
- [ ] Performance improved (include metrics)
- [ ] Minor regression (justified because...)

## Breaking Changes

<!-- If applicable, explain what breaks and how to migrate -->

## Screenshots / Output

<!-- Terminal output, screenshots, or GIFs demonstrating the change -->

## Checklist

- [ ] Code compiles without errors (`cmake --build build/windows-debug`)
- [ ] All tests pass (`ctest --preset windows-debug`)
- [ ] No compiler warnings
- [ ] Code is formatted (`clang-format -i <files>`)
- [ ] Code follows project style guidelines (see CONTRIBUTING.md)
- [ ] Self-review completed
- [ ] Complex code is commented
- [ ] Documentation updated (if needed)

## Reviewer Notes

<!-- Areas where you want specific feedback -->
