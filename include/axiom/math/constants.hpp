#pragma once

#include <limits>
#include <numbers>

namespace axiom::math {

/// Mathematical constant PI
template <typename T>
inline constexpr T PI = std::numbers::pi_v<T>;

/// Mathematical constant 2*PI
template <typename T>
inline constexpr T TWO_PI = T(2) * PI<T>;

/// Mathematical constant PI/2
template <typename T>
inline constexpr T HALF_PI = PI<T> / T(2);

/// Degrees to radians conversion factor
template <typename T>
inline constexpr T DEG_TO_RAD = PI<T> / T(180);

/// Radians to degrees conversion factor
template <typename T>
inline constexpr T RAD_TO_DEG = T(180) / PI<T>;

/// Default epsilon for floating point comparisons
template <typename T>
inline constexpr T EPSILON = std::numeric_limits<T>::epsilon();

/// Float specialization of PI
inline constexpr float PI_F = PI<float>;

/// Float specialization of 2*PI
inline constexpr float TWO_PI_F = TWO_PI<float>;

/// Float specialization of PI/2
inline constexpr float HALF_PI_F = HALF_PI<float>;

/// Float specialization of epsilon
inline constexpr float EPSILON_F = EPSILON<float>;

}  // namespace axiom::math
