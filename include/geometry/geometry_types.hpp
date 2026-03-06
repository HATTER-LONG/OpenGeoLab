/**
 * @file geometry_types.hpp
 * @brief Core geometry types and ID system for OpenGeoLab
 *
 * This file defines the fundamental geometric primitives
 * and the dual ID system used throughout the geometry layer:
 * - EntityId: Global unique identifier across all entity types
 * - EntityUID: Type-scoped unique identifier within the same entity type
 */

#pragma once
#include "util/core_identity.hpp"
#include "util/point_vector3d.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>

namespace OpenGeoLab::Geometry {

// =============================================================================
// Entity Type Definitions
// =============================================================================

/**
 * @brief Enumeration of geometric entity types
 *
 * Used for type identification and selection mode filtering.
 *
 */
enum class EntityType : uint8_t {
    Vertex = 0,    ///< Point/vertex entity
    Edge = 1,      ///< Edge/curve entity
    Wire = 2,      ///< Wire entity (collection of connected edges)
    Face = 3,      ///< Face/surface entity
    Shell = 4,     ///< Shell entity (collection of connected faces)
    Solid = 5,     ///< Solid body entity
    CompSolid = 6, ///< Composite solid entity
    Compound = 7,  ///< Compound entity (collection of shapes)
    Part = 8,      ///< UI-level part (independent component)
    None = 9,      ///< No entity type / invalid
};

/**
 * @brief Convert string to EntityType
 * @param value String representation of entity type
 * @return Optional EntityType enumeration value, or std::nullopt if invalid
 */
[[nodiscard]] std::optional<EntityType> entityTypeFromString(std::string_view value) noexcept;

/**
 * @brief Convert EntityType to string
 * @param type EntityType value
 * @return Corresponding type name string, or std::nullopt if invalid
 */
[[nodiscard]] std::optional<std::string> entityTypeToString(EntityType type) noexcept;

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
 * @brief Get the maximum assigned EntityUID for a specific type
 * @param type The entity type to query
 * @return The maximum assigned EntityUID for the given type
 */
[[nodiscard]] uint64_t getMaxIdByType(EntityType type);

/**
 * @brief Reset UID generator for a specific type (for testing purposes)
 * @param type The entity type to reset
 * @warning This function is intended for testing only
 */
void resetEntityUIDGenerator(EntityType type);

/**
 * @brief Reset all EntityUID generators (for testing purposes)
 * @warning This function is intended for testing only
 */
void resetAllEntityUIDGenerators();

/**
 * @brief Reset global EntityId generator (for testing purposes)
 * @warning This function is intended for testing only
 */
void resetEntityIdGenerator();

// =============================================================================
// EntityKey (id+uid+type)
// =============================================================================

using EntityKey = Util::CoreIdentity<EntityId,
                                     EntityUID,
                                     EntityType,
                                     INVALID_ENTITY_ID,
                                     INVALID_ENTITY_UID,
                                     EntityType::None>;
using EntityKeyHash = Util::CoreIdentityHash<EntityKey>;
using EntityKeySet = std::unordered_set<EntityKey, EntityKeyHash>;
template <typename T> using EntityKeyMap = std::unordered_map<EntityKey, T, EntityKeyHash>;
// =============================================================================
// EntityRef (uid+type only)
// =============================================================================
using EntityRef =
    Util::CoreUidIdentity<EntityUID, EntityType, INVALID_ENTITY_UID, EntityType::None>;
using EntityRefHash = Util::CoreUidIdentityHash<EntityRef>;
using EntityRefSet = std::unordered_set<EntityRef, EntityRefHash>;
template <typename T> using EntityRefMap = std::unordered_map<EntityRef, T, EntityRefHash>;

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
    Util::Pt3d m_min{std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
                     std::numeric_limits<double>::max()}; ///< Minimum corner
    Util::Pt3d m_max{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(),
                     std::numeric_limits<double>::lowest()}; ///< Maximum corner

    /// Default constructor creates an invalid (empty) box
    BoundingBox3D() = default;

    /**
     * @brief Construct from min and max corners
     * @param min_pt Minimum corner point
     * @param max_pt Maximum corner point
     */
    BoundingBox3D(const Util::Pt3d& min_pt, const Util::Pt3d& max_pt)
        : m_min(min_pt), m_max(max_pt) {}

    /**
     * @brief Check if bounding box is valid (non-empty)
     * @return true if max >= min in all dimensions
     */
    [[nodiscard]] bool isValid() const {
        return m_max.x >= m_min.x && m_max.y >= m_min.y && m_max.z >= m_min.z;
    }

    /**
     * @brief Expand box to include a point
     * @param point Point to include
     */
    void expand(const Util::Pt3d& point) {
        m_min.x = std::min(m_min.x, point.x);
        m_min.y = std::min(m_min.y, point.y);
        m_min.z = std::min(m_min.z, point.z);
        m_max.x = std::max(m_max.x, point.x);
        m_max.y = std::max(m_max.y, point.y);
        m_max.z = std::max(m_max.z, point.z);
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
    [[nodiscard]] Util::Pt3d center() const {
        return Util::Pt3d((m_min.x + m_max.x) * 0.5, (m_min.y + m_max.y) * 0.5,
                          (m_min.z + m_max.z) * 0.5);
    }

    /**
     * @brief Get box dimensions
     * @return Vector from min to max corner
     */
    [[nodiscard]] Util::Vec3d size() const {
        return Util::Vec3d(m_max.x - m_min.x, m_max.y - m_min.y, m_max.z - m_min.z);
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
    [[nodiscard]] bool contains(const Util::Pt3d& point) const {
        return point.x >= m_min.x && point.x <= m_max.x && point.y >= m_min.y &&
               point.y <= m_max.y && point.z >= m_min.z && point.z <= m_max.z;
    }

    /**
     * @brief Check if two boxes intersect
     * @param other Other bounding box
     * @return true if boxes overlap
     */
    [[nodiscard]] bool intersects(const BoundingBox3D& other) const {
        return m_min.x <= other.m_max.x && m_max.x >= other.m_min.x && m_min.y <= other.m_max.y &&
               m_max.y >= other.m_min.y && m_min.z <= other.m_max.z && m_max.z >= other.m_min.z;
    }
};

} // namespace OpenGeoLab::Geometry