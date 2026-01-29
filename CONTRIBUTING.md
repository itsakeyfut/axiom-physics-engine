# Contributing to Axiom Physics Engine

Thank you for your interest in contributing to Axiom! This document provides guidelines for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Commit Guidelines](#commit-guidelines)
- [Pull Request Process](#pull-request-process)
- [Testing Requirements](#testing-requirements)

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow
- Maintain professional communication

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/YOUR_USERNAME/axiom.git`
3. Set up vcpkg: `set VCPKG_ROOT=C:\path\to\vcpkg` (Windows)
4. Build the project: `cmake --preset windows-debug && cmake --build build/windows-debug`
5. Run tests: `ctest --preset windows-debug`

## Development Workflow

### Branch Naming Convention

- `feature/issue-XXX-description` - New features
- `bugfix/issue-XXX-description` - Bug fixes
- `refactor/description` - Code refactoring
- `docs/description` - Documentation updates
- `test/description` - Test additions or improvements

### Working on an Issue

1. Create a new branch from `main`
2. Implement your changes
3. Write or update tests
4. Ensure all tests pass
5. Update documentation if needed
6. Push your branch and create a pull request

## Coding Standards

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| **Files** | snake_case | `rigid_body.hpp`, `vec3.cpp` |
| **Classes/Structs** | PascalCase | `RigidBody`, `PhysicsWorld` |
| **Functions/Methods** | camelCase | `getPosition()`, `setVelocity()` |
| **Member Variables** | camelCase + trailing `_` | `position_`, `velocity_` |
| **Local Variables** | camelCase | `deltaTime`, `contactPoint` |
| **Constants** | UPPER_SNAKE_CASE | `MAX_CONTACTS`, `DEFAULT_GRAVITY` |
| **Namespaces** | lowercase | `axiom::core`, `axiom::collision` |
| **Template Parameters** | PascalCase | `template<typename T>` |

### Formatting

- **Indentation**: 4 spaces (no tabs)
- **Brace Style**: K&R (opening brace on same line)
  ```cpp
  void function() {
      if (condition) {
          // code
      }
  }
  ```
- **Line Length**: Aim for 100 columns, hard limit at 120
- **Header Guards**: Use `#pragma once`
- **Pointer/Reference**: Attach to type: `int* ptr`, `const T& ref`

### Include Order

Include files in the following order, with a blank line between groups:

1. Corresponding header (for .cpp files)
2. Project headers (`axiom/...`)
3. Third-party headers (`glm/...`, `spdlog/...`)
4. Standard library headers (`<vector>`, `<memory>`)

Example:
```cpp
#include "axiom/core/rigid_body.hpp"  // Corresponding header

#include "axiom/math/vec3.hpp"        // Project headers
#include "axiom/collision/shape.hpp"

#include <glm/glm.hpp>                // Third-party

#include <memory>                     // Standard library
#include <vector>
```

### Code Style

**Classes:**
```cpp
class RigidBody {
public:
    RigidBody(float mass);
    ~RigidBody() = default;

    void applyForce(const Vec3& force);
    Vec3 getPosition() const { return position_; }

private:
    Vec3 position_;
    Vec3 velocity_;
    float mass_;
};
```

**Namespaces:**
```cpp
namespace axiom {
namespace collision {

class Shape {
    // ...
};

}  // namespace collision
}  // namespace axiom
```

**Constants:**
```cpp
namespace axiom::physics {
    constexpr float DEFAULT_GRAVITY = -9.81f;
    constexpr int MAX_CONTACTS = 256;
}
```

**Auto Usage:**
- Use `auto` when the type is obvious or verbose
- Prefer explicit types for clarity when type isn't obvious

**Comments:**
- Use `//` for single-line comments
- Use `/** ... */` for documentation comments
- Write self-documenting code; comment *why*, not *what*

### Modern C++ Best Practices

- Use C++20 features when appropriate
- Prefer `constexpr` over `#define` for constants
- Use `nullptr` instead of `NULL` or `0`
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers when managing ownership
- Use range-based for loops when possible
- Use `enum class` instead of plain `enum`
- Mark functions `const`, `constexpr`, `noexcept` when applicable
- Use `override` for virtual function overrides
- Use `= default` and `= delete` for special member functions

## Commit Guidelines

### Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `refactor`: Code refactoring
- `perf`: Performance improvement
- `test`: Adding or updating tests
- `docs`: Documentation changes
- `style`: Code style changes (formatting)
- `chore`: Maintenance tasks

**Scope** (optional): The module affected (core, math, collision, dynamics, etc.)

**Examples:**
```
feat(math): add quaternion slerp implementation

Implement spherical linear interpolation for smooth rotation interpolation.
Uses the robust formulation to handle edge cases.

Closes #42
```

```
fix(collision): correct GJK termination condition

The previous implementation could miss collisions in edge cases.
Update the epsilon comparison to use relative tolerance.
```

### Guidelines

- Use imperative mood: "add feature" not "added feature"
- Keep subject line under 72 characters
- Capitalize the subject line
- No period at the end of subject line
- Separate subject from body with a blank line
- Wrap body at 72 characters
- Use body to explain *what* and *why*, not *how*
- Reference issues and PRs in footer

## Pull Request Process

1. **Update your branch** with latest `main`:
   ```bash
   git fetch origin
   git rebase origin/main
   ```

2. **Ensure all tests pass**:
   ```bash
   ctest --preset windows-debug
   ```

3. **Run code formatter**:
   ```bash
   clang-format -i <changed_files>
   ```

4. **Create pull request** with:
   - Clear title describing the change
   - Description of what changed and why
   - Reference to related issues (e.g., "Closes #123")
   - Screenshots or benchmarks if applicable

5. **PR Checklist**:
   - [ ] Code follows project style guidelines
   - [ ] All tests pass
   - [ ] New tests added for new functionality
   - [ ] Documentation updated if needed
   - [ ] No compiler warnings
   - [ ] Commit messages follow guidelines

6. **Code Review**:
   - Address all review comments
   - Push additional commits or amend as requested
   - Mark conversations as resolved after addressing

7. **Merge**:
   - PRs require at least one approval
   - CI must pass before merging
   - Use "Squash and merge" for feature branches
   - Use "Rebase and merge" for clean, atomic commits

## Testing Requirements

### Unit Tests

- Write tests for all new functionality
- Aim for 80%+ code coverage
- Use Google Test framework
- Place tests in `tests/` directory mirroring source structure

Example:
```cpp
#include <gtest/gtest.h>
#include "axiom/math/vec3.hpp"

namespace axiom::math::test {

TEST(Vec3Test, DefaultConstructor) {
    Vec3 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST(Vec3Test, DotProduct) {
    Vec3 a(1.0f, 0.0f, 0.0f);
    Vec3 b(0.0f, 1.0f, 0.0f);
    EXPECT_FLOAT_EQ(dot(a, b), 0.0f);
}

}  // namespace axiom::math::test
```

### Benchmarks

- Add benchmarks for performance-critical code
- Use Google Benchmark framework
- Place benchmarks in `benchmarks/` directory

### Running Tests

```bash
# All tests
ctest --preset windows-debug

# Specific test
./build/windows-debug/bin/axiom_tests --gtest_filter="Vec3Test.*"

# With verbose output
ctest --preset windows-debug --verbose

# Run benchmarks
./build/windows-release/bin/axiom_benchmark
```

## Documentation

- Document all public APIs with Doxygen-style comments
- Include usage examples for complex functionality
- Update README.md for user-facing changes
- Keep CLAUDE.md updated with architectural decisions

Example:
```cpp
/**
 * @brief Applies a force to the rigid body.
 *
 * The force is accumulated and applied during the next physics step.
 * Forces are automatically cleared after each step.
 *
 * @param force The force vector in world space (Newtons)
 * @param point Optional point of application in world space
 *
 * @note This uses force accumulation, not impulse
 */
void applyForce(const Vec3& force, const Vec3& point = Vec3::zero());
```

## Questions?

- Check existing documentation in `docs/`
- Search existing issues and pull requests
- Create a new issue with the `question` label
- Join discussions in existing issues

Thank you for contributing to Axiom Physics Engine!
