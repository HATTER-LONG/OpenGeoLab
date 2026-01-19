/**
 * @file geometry_types.hpp
 * @brief Core geometry type definitions and enumerations for OpenGeoLab
 *
 * Defines fundamental types used throughout the geometry subsystem,
 * including selection modes, entity types, and common data structures.
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief Types of geometric entities that can be selected
 */
enum class EntityType : uint8_t {
    None = 0,    ///< No entity type
    Vertex = 1,  ///< Point/vertex entity
    Edge = 2,    ///< Edge/curve entity
    Face = 3,    ///< Face/surface entity
    Solid = 4,   ///< Solid body entity
    Shell = 5,   ///< Shell entity (collection of faces)
    Wire = 6,    ///< Wire entity (collection of edges)
    Compound = 7 ///< Compound entity (collection of shapes)
};

/**
 * @brief Selection mode for geometry picking operations
 */
enum class SelectionMode : uint8_t {
    None = 0,   ///< Selection disabled
    Vertex = 1, ///< Select vertices only
    Edge = 2,   ///< Select edges only
    Face = 3,   ///< Select faces only
    Solid = 4,  ///< Select solid bodies only
    Part = 5,   ///< Select entire parts
    Multi = 6   ///< Multi-selection mode
};

/**
 * @brief 3D point representation
 */
struct Point3D {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    Point3D() = default;
    Point3D(double px, double py, double pz) : x(px), y(py), z(pz) {}

    bool operator==(const Point3D& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

/**
 * @brief 3D vector representation
 */
struct Vector3D {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    Vector3D() = default;
    Vector3D(double vx, double vy, double vz) : x(vx), y(vy), z(vz) {}

    [[nodiscard]] double length() const;
    [[nodiscard]] Vector3D normalized() const;
};

/**
 * @brief Color representation with RGBA components
 */
struct Color {
    float r{0.8f};
    float g{0.8f};
    float b{0.8f};
    float a{1.0f};

    Color() = default;
    Color(float red, float green, float blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}

    /**
     * @brief Create color from integer RGB values (0-255)
     */
    static Color fromRGB(int red, int green, int blue, int alpha = 255) {
        return Color(static_cast<float>(red) / 255.0f, static_cast<float>(green) / 255.0f,
                     static_cast<float>(blue) / 255.0f, static_cast<float>(alpha) / 255.0f);
    }
};

/**
 * @brief Axis-aligned bounding box
 */
struct BoundingBox {
    Point3D min;
    Point3D max;

    BoundingBox() = default;
    BoundingBox(const Point3D& minPt, const Point3D& maxPt) : min(minPt), max(maxPt) {}

    /**
     * @brief Calculate the center point of the bounding box
     */
    [[nodiscard]] Point3D center() const {
        return Point3D((min.x + max.x) / 2.0, (min.y + max.y) / 2.0, (min.z + max.z) / 2.0);
    }

    /**
     * @brief Calculate the diagonal length of the bounding box
     */
    [[nodiscard]] double diagonalLength() const;

    /**
     * @brief Check if the bounding box is valid (min < max)
     */
    [[nodiscard]] bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    /**
     * @brief Expand the bounding box to include a point
     */
    void expand(const Point3D& point);

    /**
     * @brief Expand the bounding box to include another bounding box
     */
    void expand(const BoundingBox& other);
};

/**
 * @brief Unique identifier for geometric entities
 */
using EntityId = uint64_t;

/**
 * @brief Invalid entity ID constant
 */
constexpr EntityId INVALID_ENTITY_ID = 0;

/**
 * @brief Generate a unique entity ID
 */
EntityId generateEntityId();

} // namespace OpenGeoLab::Geometry
