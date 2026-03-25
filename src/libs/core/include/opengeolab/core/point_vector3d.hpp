/**
 * @file point_vector3d.hpp
 * @brief 3D point and vector types with arithmetic, geometric operations, and
 *        fuzzy comparison utilities
 */

#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace OpenGeoLab::Core {
constexpr double DEFAULT_EPSILON = 1e-8;

constexpr double DEFAULT_REL_EPSILON = 1e-6;
constexpr double DEFAULT_ABS_EPSILON = 1e-12;

/**
 * @brief Compare two arithmetic values with combined relative/absolute tolerance
 * @tparam T Arithmetic type; floating-point uses fuzzy comparison, integers use exact equality
 * @param a First value
 * @param b Second value
 * @param rel_eps Maximum relative error relative to the larger magnitude
 * @param abs_eps Maximum absolute error for near-zero comparisons
 * @return true if values are considered equal within tolerances
 */
template <class T>
constexpr bool almostEqual(T a,
                           T b,
                           T rel_eps = T(DEFAULT_REL_EPSILON),
                           T abs_eps = T(DEFAULT_ABS_EPSILON)) noexcept {
    if constexpr(std::is_floating_point_v<T>) {
        const T diff = std::abs(a - b);
        if(diff <= abs_eps) {
            return true;
        }
        return diff <= rel_eps * std::max(std::abs(a), std::abs(b));
    } else {
        return a == b;
    }
}

/**
 * @brief 3D vector with arithmetic, dot/cross product, normalization, and
 *        fuzzy comparison
 * @tparam T Arithmetic element type (float, double, int, etc.)
 */
template <class T> struct Vector3D final {
    static_assert(std::is_arithmetic_v<T>, "Vector3D<T>: T must be arithmetic.");

    T x{}; // NOLINT
    T y{}; // NOLINT
    T z{}; // NOLINT

    constexpr Vector3D() noexcept = default;
    constexpr Vector3D(T x, T y, T z) noexcept : x(x), y(y), z(z) {}

    constexpr T& operator[](std::size_t i) noexcept { return i == 0 ? x : (i == 1 ? y : z); }
    constexpr const T& operator[](std::size_t i) const noexcept {
        return i == 0 ? x : (i == 1 ? y : z);
    }

    constexpr Vector3D operator+() const noexcept { return *this; }
    constexpr Vector3D operator-() const noexcept { return {-x, -y, -z}; }

    constexpr Vector3D& operator+=(const Vector3D& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    constexpr Vector3D& operator-=(const Vector3D& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    constexpr Vector3D& operator*=(T s) noexcept {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    constexpr Vector3D& operator/=(T s) noexcept {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }

    /** @brief Dot (inner) product */
    [[nodiscard]] constexpr T dot(const Vector3D& rhs) const noexcept {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    /** @brief Cross product (right-hand rule) */
    [[nodiscard]] constexpr Vector3D cross(const Vector3D& rhs) const noexcept {
        return {y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x};
    }

    [[nodiscard]] constexpr T squaredLength() const noexcept { return dot(*this); }

    [[nodiscard]] T length() const noexcept {
        if constexpr(std::is_floating_point_v<T>) {
            return std::sqrt(squaredLength());
        } else {
            return static_cast<T>(std::sqrt(static_cast<long double>(squaredLength())));
        }
    }

    /**
     * @brief Return a unit-length copy; returns zero vector if length <= eps
     */
    [[nodiscard]] Vector3D normalized(T eps = defaultEpsilon()) const noexcept {
        if constexpr(std::is_floating_point_v<T>) {
            const T len2 = squaredLength();
            if(len2 <= eps * eps) {
                return Vector3D{};
            }
            const T inv = T(1) / std::sqrt(len2);
            return (*this) * inv;
        } else {
            return *this;
        }
    }

    /**
     * @brief Normalize in place
     * @return true on success; false (and unchanged) if length <= eps
     */
    bool normalizeInplace(T eps = defaultEpsilon()) noexcept {
        if constexpr(std::is_floating_point_v<T>) {
            const T len2 = squaredLength();
            if(len2 <= eps * eps) {
                return false;
            }
            const T inv = T(1) / std::sqrt(len2);
            (*this) *= inv;
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] constexpr bool almostEquals(const Vector3D& rhs,
                                              T rel_eps = T(DEFAULT_REL_EPSILON),
                                              T abs_eps = T(DEFAULT_ABS_EPSILON)) const noexcept {
        return almostEqual(x, rhs.x, rel_eps, abs_eps) && almostEqual(y, rhs.y, rel_eps, abs_eps) &&
               almostEqual(z, rhs.z, rel_eps, abs_eps);
    }

    static constexpr T defaultEpsilon() noexcept {
        if constexpr(std::is_floating_point_v<T>) {
            return T(DEFAULT_EPSILON);
        } else {
            return T(0);
        }
    }
};

template <class T>
[[nodiscard]] constexpr Vector3D<T> operator+(Vector3D<T> a, const Vector3D<T>& b) noexcept {
    a += b;
    return a;
}
template <class T>
[[nodiscard]] constexpr Vector3D<T> operator-(Vector3D<T> a, const Vector3D<T>& b) noexcept {
    a -= b;
    return a;
}
template <class T> [[nodiscard]] constexpr Vector3D<T> operator*(Vector3D<T> v, T s) noexcept {
    v *= s;
    return v;
}
template <class T> [[nodiscard]] constexpr Vector3D<T> operator*(T s, Vector3D<T> v) noexcept {
    v *= s;
    return v;
}
template <class T> [[nodiscard]] constexpr Vector3D<T> operator/(Vector3D<T> v, T s) noexcept {
    v /= s;
    return v;
}

/**
 * @brief 3D point with distance computation and fuzzy comparison
 * @tparam T Arithmetic element type (float, double, int, etc.)
 */
template <class T> struct Point3D final {
    static_assert(std::is_arithmetic_v<T>, "Point3D<T>: T must be arithmetic.");

    T x{}; // NOLINT
    T y{}; // NOLINT
    T z{}; // NOLINT

    constexpr Point3D() noexcept = default;
    constexpr Point3D(T x, T y, T z) noexcept : x(x), y(y), z(z) {}

    constexpr T& operator[](std::size_t i) noexcept { return i == 0 ? x : (i == 1 ? y : z); }
    constexpr const T& operator[](std::size_t i) const noexcept {
        return i == 0 ? x : (i == 1 ? y : z);
    }

    [[nodiscard]] T distanceTo(const Point3D& rhs) const noexcept { return (rhs - *this).length(); }

    [[nodiscard]] constexpr T squaredDistanceTo(const Point3D& rhs) const noexcept {
        return (rhs - *this).squaredLength();
    }

    [[nodiscard]] constexpr bool
    almostEquals(const Point3D& rhs, T rel_eps = T(1e-6), T abs_eps = T(1e-12)) const noexcept {
        return almostEqual(x, rhs.x, rel_eps, abs_eps) && almostEqual(y, rhs.y, rel_eps, abs_eps) &&
               almostEqual(z, rhs.z, rel_eps, abs_eps);
    }
};

template <class T>
[[nodiscard]] constexpr Point3D<T> operator+(Point3D<T> p, const Vector3D<T>& v) noexcept {
    p.x += v.x;
    p.y += v.y;
    p.z += v.z;
    return p;
}
template <class T>
[[nodiscard]] constexpr Point3D<T> operator+(const Vector3D<T>& v, Point3D<T> p) noexcept {
    return p + v;
}

template <class T>
[[nodiscard]] constexpr Point3D<T> operator-(Point3D<T> p, const Vector3D<T>& v) noexcept {
    p.x -= v.x;
    p.y -= v.y;
    p.z -= v.z;
    return p;
}

template <class T>
[[nodiscard]] constexpr Vector3D<T> operator-(const Point3D<T>& a, const Point3D<T>& b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

template <class T> constexpr Point3D<T>& operator+=(Point3D<T>& p, const Vector3D<T>& v) noexcept {
    p = p + v;
    return p;
}
template <class T> constexpr Point3D<T>& operator-=(Point3D<T>& p, const Vector3D<T>& v) noexcept {
    p = p - v;
    return p;
}

using Vec3f = Vector3D<float>;
using Vec3d = Vector3D<double>;
using Pt3f = Point3D<float>;
using Pt3d = Point3D<double>;

} // namespace OpenGeoLab::Core