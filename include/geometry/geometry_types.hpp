/**
 * @file geometry_types.hpp
 * @brief Core geometry types and ID system for OpenGeoLab
 *
 * This file defines the fundamental geometric primitives (Point3D, Vector3D, etc.)
 * and the dual ID system used throughout the geometry layer:
 * - EntityId: Global unique identifier across all entity types
 * - EntityUID: Type-scoped unique identifier within the same entity type
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace OpenGeoLab::Geometry {

// =============================================================================
// Entity Type Definitions
// =============================================================================

/**
 * @brief Enumeration of geometric entity types
 *
 * Used for type identification and selection mode filtering.
 */
enum class EntityType : uint8_t {
    None = 0,      ///< No entity type / invalid
    Vertex = 1,    ///< Point/vertex entity
    Edge = 2,      ///< Edge/curve entity
    Wire = 3,      ///< Wire entity (collection of connected edges)
    Face = 4,      ///< Face/surface entity
    Shell = 5,     ///< Shell entity (collection of connected faces)
    Solid = 6,     ///< Solid body entity
    CompSolid = 7, ///< Composite solid entity
    Compound = 8   ///< Compound entity (collection of shapes)
};

/**
 * @brief Selection mode for interactive geometry picking
 */
enum class SelectionMode : uint8_t {
    None = 0,      ///< Selection disabled
    Vertex = 1,    ///< Select vertices only
    Edge = 2,      ///< Select edges only
    Face = 3,      ///< Select faces only
    Solid = 4,     ///< Select solid bodies only
    CompSolid = 5, ///< Select composite solids only
    Compound = 6,  ///< Select compounds only
    Multi = 7      ///< Multi-selection mode (multiple types)
};

// =============================================================================
// ID System
// =============================================================================

/**
 * @brief Global unique identifier for any geometry entity
 *
 * EntityId provides a globally unique identifier across all entity types.
 * It can be used to quickly locate any entity in the geometry system.
 */
using EntityId = uint64_t;

/**
 * @brief Type-scoped unique identifier within the same entity type
 *
 * EntityUID is unique within the same EntityType. For example, vertex UID 1
 * and edge UID 1 are different entities. Combined with EntityType, it forms
 * a complete entity reference.
 */
using EntityUID = uint64_t;

/// Invalid/null EntityId constant
constexpr EntityId INVALID_ENTITY_ID = 0;

/// Invalid/null EntityUID constant
constexpr EntityUID INVALID_ENTITY_UID = 0;

/**
 * @brief Generate a new globally unique EntityId
 * @return A new unique EntityId (thread-safe)
 */
[[nodiscard]] EntityId generateEntityId();

/**
 * @brief Generate a new type-scoped EntityUID
 * @param type The entity type for which to generate a UID
 * @return A new unique EntityUID for the given type (thread-safe)
 */
[[nodiscard]] EntityUID generateEntityUID(EntityType type);

/**
 * @brief Reset UID generator for a specific type (for testing purposes)
 * @param type The entity type to reset
 * @warning This function is intended for testing only
 */
void resetEntityUIDGenerator(EntityType type);

/**
 * @brief Reset global EntityId generator (for testing purposes)
 * @warning This function is intended for testing only
 */
void resetEntityIdGenerator();

// =============================================================================
// Geometric Tolerance
// =============================================================================

/// Default geometric tolerance for comparison operations
constexpr double DEFAULT_TOLERANCE = 1e-9;

/**
 * @brief Check if two floating-point values are approximately equal
 * @param a First value
 * @param b Second value
 * @param tolerance Comparison tolerance (default: DEFAULT_TOLERANCE)
 * @return true if |a - b| < tolerance
 */
[[nodiscard]] inline bool isApproxEqual(double a, double b, double tolerance = DEFAULT_TOLERANCE) {
    return std::fabs(a - b) < tolerance;
}

// =============================================================================
// Point3D
// =============================================================================

/**
 * @brief 3D point in Cartesian coordinates
 *
 * Represents a position in 3D space with x, y, z coordinates.
 */
struct Point3D {
    double m_x{0.0}; ///< X coordinate
    double m_y{0.0}; ///< Y coordinate
    double m_z{0.0}; ///< Z coordinate

    /// Default constructor, initializes to origin (0, 0, 0)
    Point3D() = default;

    /**
     * @brief Construct a point from coordinates
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     */
    Point3D(double x, double y, double z) : m_x(x), m_y(y), m_z(z) {}

    /// Exact equality comparison
    [[nodiscard]] bool operator==(const Point3D& other) const {
        return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
    }

    /// Exact inequality comparison
    [[nodiscard]] bool operator!=(const Point3D& other) const { return !(*this == other); }

    /// Point addition (translation)
    [[nodiscard]] Point3D operator+(const Point3D& other) const {
        return Point3D(m_x + other.m_x, m_y + other.m_y, m_z + other.m_z);
    }

    /// Point subtraction (displacement vector)
    [[nodiscard]] Point3D operator-(const Point3D& other) const {
        return Point3D(m_x - other.m_x, m_y - other.m_y, m_z - other.m_z);
    }

    /// Scalar multiplication
    [[nodiscard]] Point3D operator*(double scalar) const {
        return Point3D(m_x * scalar, m_y * scalar, m_z * scalar);
    }

    /// Scalar division
    [[nodiscard]] Point3D operator/(double scalar) const {
        return Point3D(m_x / scalar, m_y / scalar, m_z / scalar);
    }

    /**
     * @brief Check approximate equality with tolerance
     * @param other Point to compare with
     * @param tolerance Comparison tolerance
     * @return true if points are within tolerance distance
     */
    [[nodiscard]] bool isApprox(const Point3D& other, double tolerance = DEFAULT_TOLERANCE) const {
        return isApproxEqual(m_x, other.m_x, tolerance) &&
               isApproxEqual(m_y, other.m_y, tolerance) && isApproxEqual(m_z, other.m_z, tolerance);
    }

    /**
     * @brief Calculate Euclidean distance to another point
     * @param other Target point
     * @return Distance between this point and other
     */
    [[nodiscard]] double distanceTo(const Point3D& other) const {
        const double dx = m_x - other.m_x;
        const double dy = m_y - other.m_y;
        const double dz = m_z - other.m_z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /**
     * @brief Calculate squared distance to another point
     * @param other Target point
     * @return Squared distance (faster than distanceTo when comparing distances)
     */
    [[nodiscard]] double squaredDistanceTo(const Point3D& other) const {
        const double dx = m_x - other.m_x;
        const double dy = m_y - other.m_y;
        const double dz = m_z - other.m_z;
        return dx * dx + dy * dy + dz * dz;
    }

    /**
     * @brief Linear interpolation between two points
     * @param other Target point
     * @param t Interpolation parameter [0, 1]
     * @return Interpolated point: this * (1-t) + other * t
     */
    [[nodiscard]] Point3D lerp(const Point3D& other, double t) const {
        return Point3D(m_x + (other.m_x - m_x) * t, m_y + (other.m_y - m_y) * t,
                       m_z + (other.m_z - m_z) * t);
    }

    /// Create origin point (0, 0, 0)
    [[nodiscard]] static Point3D origin() { return Point3D(0.0, 0.0, 0.0); }
};

// =============================================================================
// Vector3D
// =============================================================================

/**
 * @brief 3D vector for directions and displacements
 *
 * Represents a direction and magnitude in 3D space.
 * Provides common vector operations: dot product, cross product, normalization, etc.
 */
struct Vector3D {
    double m_x{0.0}; ///< X component
    double m_y{0.0}; ///< Y component
    double m_z{0.0}; ///< Z component

    /// Default constructor, initializes to zero vector
    Vector3D() = default;

    /**
     * @brief Construct a vector from components
     * @param x X component
     * @param y Y component
     * @param z Z component
     */
    Vector3D(double x, double y, double z) : m_x(x), m_y(y), m_z(z) {}

    /**
     * @brief Construct a vector from a Point3D (position vector from origin)
     * @param point Source point
     */
    explicit Vector3D(const Point3D& point) : m_x(point.m_x), m_y(point.m_y), m_z(point.m_z) {}

    /// Exact equality comparison
    [[nodiscard]] bool operator==(const Vector3D& other) const {
        return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
    }

    /// Exact inequality comparison
    [[nodiscard]] bool operator!=(const Vector3D& other) const { return !(*this == other); }

    /// Vector addition
    [[nodiscard]] Vector3D operator+(const Vector3D& other) const {
        return Vector3D(m_x + other.m_x, m_y + other.m_y, m_z + other.m_z);
    }

    /// Vector subtraction
    [[nodiscard]] Vector3D operator-(const Vector3D& other) const {
        return Vector3D(m_x - other.m_x, m_y - other.m_y, m_z - other.m_z);
    }

    /// Unary negation
    [[nodiscard]] Vector3D operator-() const { return Vector3D(-m_x, -m_y, -m_z); }

    /// Scalar multiplication
    [[nodiscard]] Vector3D operator*(double scalar) const {
        return Vector3D(m_x * scalar, m_y * scalar, m_z * scalar);
    }

    /// Scalar division
    [[nodiscard]] Vector3D operator/(double scalar) const {
        return Vector3D(m_x / scalar, m_y / scalar, m_z / scalar);
    }

    /// In-place addition
    Vector3D& operator+=(const Vector3D& other) {
        m_x += other.m_x;
        m_y += other.m_y;
        m_z += other.m_z;
        return *this;
    }

    /// In-place subtraction
    Vector3D& operator-=(const Vector3D& other) {
        m_x -= other.m_x;
        m_y -= other.m_y;
        m_z -= other.m_z;
        return *this;
    }

    /// In-place scalar multiplication
    Vector3D& operator*=(double scalar) {
        m_x *= scalar;
        m_y *= scalar;
        m_z *= scalar;
        return *this;
    }

    /// In-place scalar division
    Vector3D& operator/=(double scalar) {
        m_x /= scalar;
        m_y /= scalar;
        m_z /= scalar;
        return *this;
    }

    /**
     * @brief Compute dot product with another vector
     * @param other Second vector
     * @return Scalar dot product (a·b = |a||b|cos(θ))
     */
    [[nodiscard]] double dot(const Vector3D& other) const {
        return m_x * other.m_x + m_y * other.m_y + m_z * other.m_z;
    }

    /**
     * @brief Compute cross product with another vector
     * @param other Second vector
     * @return Cross product vector (a×b, perpendicular to both a and b)
     */
    [[nodiscard]] Vector3D cross(const Vector3D& other) const {
        return Vector3D(m_y * other.m_z - m_z * other.m_y, m_z * other.m_x - m_x * other.m_z,
                        m_x * other.m_y - m_y * other.m_x);
    }

    /**
     * @brief Get vector length (magnitude)
     * @return Euclidean length: sqrt(x² + y² + z²)
     */
    [[nodiscard]] double length() const { return std::sqrt(m_x * m_x + m_y * m_y + m_z * m_z); }

    /**
     * @brief Get squared length (avoids sqrt for comparisons)
     * @return x² + y² + z²
     */
    [[nodiscard]] double squaredLength() const { return m_x * m_x + m_y * m_y + m_z * m_z; }

    /**
     * @brief Get normalized (unit length) vector
     * @return Unit vector in the same direction, or zero vector if length is zero
     */
    [[nodiscard]] Vector3D normalized() const {
        const double len = length();
        if(len < DEFAULT_TOLERANCE) {
            return Vector3D(0.0, 0.0, 0.0);
        }
        return Vector3D(m_x / len, m_y / len, m_z / len);
    }

    /**
     * @brief Normalize this vector in place
     * @return Reference to this vector after normalization
     */
    Vector3D& normalize() {
        const double len = length();
        if(len >= DEFAULT_TOLERANCE) {
            m_x /= len;
            m_y /= len;
            m_z /= len;
        } else {
            m_x = m_y = m_z = 0.0;
        }
        return *this;
    }

    /**
     * @brief Check if vector is approximately zero
     * @param tolerance Comparison tolerance
     * @return true if all components are near zero
     */
    [[nodiscard]] bool isZero(double tolerance = DEFAULT_TOLERANCE) const {
        return squaredLength() < tolerance * tolerance;
    }

    /**
     * @brief Check if vector is approximately unit length
     * @param tolerance Comparison tolerance
     * @return true if |length - 1| < tolerance
     */
    [[nodiscard]] bool isUnit(double tolerance = DEFAULT_TOLERANCE) const {
        return isApproxEqual(squaredLength(), 1.0, tolerance * 2.0);
    }

    /**
     * @brief Check approximate equality with another vector
     * @param other Vector to compare with
     * @param tolerance Comparison tolerance
     * @return true if all component differences are within tolerance
     */
    [[nodiscard]] bool isApprox(const Vector3D& other, double tolerance = DEFAULT_TOLERANCE) const {
        return isApproxEqual(m_x, other.m_x, tolerance) &&
               isApproxEqual(m_y, other.m_y, tolerance) && isApproxEqual(m_z, other.m_z, tolerance);
    }

    /**
     * @brief Calculate angle between two vectors
     * @param other Second vector
     * @return Angle in radians [0, π]
     */
    [[nodiscard]] double angleTo(const Vector3D& other) const {
        const double len_product = length() * other.length();
        if(len_product < DEFAULT_TOLERANCE) {
            return 0.0;
        }
        const double cos_angle = std::clamp(dot(other) / len_product, -1.0, 1.0);
        return std::acos(cos_angle);
    }

    /**
     * @brief Check if vectors are parallel (same or opposite direction)
     * @param other Second vector
     * @param tolerance Comparison tolerance
     * @return true if vectors are parallel
     */
    [[nodiscard]] bool isParallelTo(const Vector3D& other,
                                    double tolerance = DEFAULT_TOLERANCE) const {
        return cross(other).isZero(tolerance);
    }

    /**
     * @brief Check if vectors are perpendicular (orthogonal)
     * @param other Second vector
     * @param tolerance Comparison tolerance
     * @return true if dot product is near zero
     */
    [[nodiscard]] bool isPerpendicularTo(const Vector3D& other,
                                         double tolerance = DEFAULT_TOLERANCE) const {
        return isApproxEqual(dot(other), 0.0, tolerance);
    }

    /**
     * @brief Project this vector onto another vector
     * @param onto Vector to project onto
     * @return Projection of this vector onto 'onto'
     */
    [[nodiscard]] Vector3D projectOnto(const Vector3D& onto) const {
        const double onto_len_sq = onto.squaredLength();
        if(onto_len_sq < DEFAULT_TOLERANCE * DEFAULT_TOLERANCE) {
            return Vector3D(0.0, 0.0, 0.0);
        }
        return onto * (dot(onto) / onto_len_sq);
    }

    /**
     * @brief Reflect this vector about a normal
     * @param normal Normal vector (should be unit length)
     * @return Reflected vector
     */
    [[nodiscard]] Vector3D reflect(const Vector3D& normal) const {
        return *this - normal * (2.0 * dot(normal));
    }

    /**
     * @brief Linear interpolation between two vectors
     * @param other Target vector
     * @param t Interpolation parameter [0, 1]
     * @return Interpolated vector: this * (1-t) + other * t
     */
    [[nodiscard]] Vector3D lerp(const Vector3D& other, double t) const {
        return Vector3D(m_x + (other.m_x - m_x) * t, m_y + (other.m_y - m_y) * t,
                        m_z + (other.m_z - m_z) * t);
    }

    /// Standard basis vectors
    [[nodiscard]] static Vector3D unitX() { return Vector3D(1.0, 0.0, 0.0); }
    [[nodiscard]] static Vector3D unitY() { return Vector3D(0.0, 1.0, 0.0); }
    [[nodiscard]] static Vector3D unitZ() { return Vector3D(0.0, 0.0, 1.0); }
    [[nodiscard]] static Vector3D zero() { return Vector3D(0.0, 0.0, 0.0); }
};

/// Scalar multiplication from left (scalar * vector)
[[nodiscard]] inline Vector3D operator*(double scalar, const Vector3D& vec) { return vec * scalar; }

// =============================================================================
// BoundingBox3D
// =============================================================================

/**
 * @brief Axis-aligned bounding box in 3D space
 *
 * Represents a rectangular box aligned with coordinate axes,
 * defined by minimum and maximum corner points.
 */
struct BoundingBox3D {
    Point3D m_min{std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
                  std::numeric_limits<double>::max()}; ///< Minimum corner
    Point3D m_max{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(),
                  std::numeric_limits<double>::lowest()}; ///< Maximum corner

    /// Default constructor creates an invalid (empty) box
    BoundingBox3D() = default;

    /**
     * @brief Construct from min and max corners
     * @param min_pt Minimum corner point
     * @param max_pt Maximum corner point
     */
    BoundingBox3D(const Point3D& min_pt, const Point3D& max_pt) : m_min(min_pt), m_max(max_pt) {}

    /**
     * @brief Check if bounding box is valid (non-empty)
     * @return true if max >= min in all dimensions
     */
    [[nodiscard]] bool isValid() const {
        return m_max.m_x >= m_min.m_x && m_max.m_y >= m_min.m_y && m_max.m_z >= m_min.m_z;
    }

    /**
     * @brief Expand box to include a point
     * @param point Point to include
     */
    void expand(const Point3D& point) {
        m_min.m_x = std::min(m_min.m_x, point.m_x);
        m_min.m_y = std::min(m_min.m_y, point.m_y);
        m_min.m_z = std::min(m_min.m_z, point.m_z);
        m_max.m_x = std::max(m_max.m_x, point.m_x);
        m_max.m_y = std::max(m_max.m_y, point.m_y);
        m_max.m_z = std::max(m_max.m_z, point.m_z);
    }

    /**
     * @brief Expand box to include another bounding box
     * @param other Box to include
     */
    void expand(const BoundingBox3D& other) {
        if(other.isValid()) {
            expand(other.m_min);
            expand(other.m_max);
        }
    }

    /**
     * @brief Get box center point
     * @return Center of the bounding box
     */
    [[nodiscard]] Point3D center() const {
        return Point3D((m_min.m_x + m_max.m_x) * 0.5, (m_min.m_y + m_max.m_y) * 0.5,
                       (m_min.m_z + m_max.m_z) * 0.5);
    }

    /**
     * @brief Get box dimensions
     * @return Vector from min to max corner
     */
    [[nodiscard]] Vector3D size() const {
        return Vector3D(m_max.m_x - m_min.m_x, m_max.m_y - m_min.m_y, m_max.m_z - m_min.m_z);
    }

    /**
     * @brief Get diagonal length
     * @return Distance from min to max corner
     */
    [[nodiscard]] double diagonal() const { return m_min.distanceTo(m_max); }

    /**
     * @brief Check if a point is inside the box
     * @param point Point to test
     * @return true if point is inside or on the boundary
     */
    [[nodiscard]] bool contains(const Point3D& point) const {
        return point.m_x >= m_min.m_x && point.m_x <= m_max.m_x && point.m_y >= m_min.m_y &&
               point.m_y <= m_max.m_y && point.m_z >= m_min.m_z && point.m_z <= m_max.m_z;
    }

    /**
     * @brief Check if two boxes intersect
     * @param other Other bounding box
     * @return true if boxes overlap
     */
    [[nodiscard]] bool intersects(const BoundingBox3D& other) const {
        return m_min.m_x <= other.m_max.m_x && m_max.m_x >= other.m_min.m_x &&
               m_min.m_y <= other.m_max.m_y && m_max.m_y >= other.m_min.m_y &&
               m_min.m_z <= other.m_max.m_z && m_max.m_z >= other.m_min.m_z;
    }
};

} // namespace OpenGeoLab::Geometry