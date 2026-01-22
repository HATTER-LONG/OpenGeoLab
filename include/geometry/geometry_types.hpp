#pragma once

#include <cmath>
#include <cstdint>
namespace OpenGeoLab::Geometry {

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

enum class SelectionMode : uint8_t {
    None = 0,   ///< Selection disabled
    Vertex = 1, ///< Select vertices only
    Edge = 2,   ///< Select edges only
    Face = 3,   ///< Select faces only
    Solid = 4,  ///< Select solid bodies only
    Part = 5,   ///< Select entire parts
    Multi = 6   ///< Multi-selection mode
};

using EntityId = uint64_t;
using EntityUID = uint64_t;

constexpr EntityId INVALID_ENTITY_ID = 0;
constexpr EntityUID INVALID_ENTITY_UID = 0;

EntityId generateEntityId();
EntityUID generateEntityUID(EntityType type);

struct Point3D {
    double m_x{0.0};
    double m_y{0.0};
    double m_z{0.0};

    Point3D() = default;
    Point3D(double x, double y, double z) : m_x(x), m_y(y), m_z(z) {}

    [[nodiscard]] bool operator==(const Point3D& other) const {
        return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
    }
    [[nodiscard]] bool operator!=(const Point3D& other) const { return !(*this == other); }
    [[nodiscard]] Point3D operator+(const Point3D& other) const {
        return Point3D(m_x + other.m_x, m_y + other.m_y, m_z + other.m_z);
    }
    [[nodiscard]] Point3D operator-(const Point3D& other) const {
        return Point3D(m_x - other.m_x, m_y - other.m_y, m_z - other.m_z);
    }
    [[nodiscard]] Point3D operator*(double scalar) const {
        return Point3D(m_x * scalar, m_y * scalar, m_z * scalar);
    }
    [[nodiscard]] Point3D operator/(double scalar) const {
        return Point3D(m_x / scalar, m_y / scalar, m_z / scalar);
    }
};

struct Vector3D {
    double m_x{0.0};
    double m_y{0.0};
    double m_z{0.0};

    Vector3D() = default;
    Vector3D(double x, double y, double z) : m_x(x), m_y(y), m_z(z) {}

    [[nodiscard]] bool operator==(const Vector3D& other) const {
        return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
    }
    [[nodiscard]] bool operator!=(const Vector3D& other) const { return !(*this == other); }
    [[nodiscard]] double dot(const Vector3D& other) const {
        return m_x * other.m_x + m_y * other.m_y + m_z * other.m_z;
    }
    [[nodiscard]] Vector3D cross(const Vector3D& other) const {
        return Vector3D(m_y * other.m_z - m_z * other.m_y, m_z * other.m_x - m_x * other.m_z,
                        m_x * other.m_y - m_y * other.m_x);
    }
    [[nodiscard]] double length() const { return std::sqrt(m_x * m_x + m_y * m_y + m_z * m_z); }
    [[nodiscard]] Vector3D normalized() const {
        double mag = length();
        if(mag == 0.0) {
            return Vector3D(0.0, 0.0, 0.0);
        }
        return Vector3D(m_x / mag, m_y / mag, m_z / mag);
    }
};

} // namespace OpenGeoLab::Geometry