/**
 * @file geometry_entity.hpp
 * @brief Geometry entity management with OCC integration
 *
 * This file provides the core geometry entity classes that wrap OpenCASCADE
 * topological shapes and provide ID-based entity management. The entity system
 * supports both global EntityId and type-scoped EntityUID for flexible querying.
 */

#pragma once

#include "geometry_types.hpp"

#include <TopExp_Explorer.hxx>
#include <TopoDS_Shape.hxx>

#include <kangaroo/util/noncopyable.hpp>

#include <memory>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Geometry {

class GeometryEntity;
class GeometryManager;

using GeometryEntityPtr = std::shared_ptr<GeometryEntity>;
using GeometryEntityWeakPtr = std::weak_ptr<GeometryEntity>;

// =============================================================================
// GeometryEntity
// =============================================================================

/**
 * @brief Base class for all geometry entities wrapping OCC shapes
 *
 * GeometryEntity provides:
 * - Dual ID system (EntityId for global, EntityUID for type-scoped)
 * - OCC TopoDS_Shape storage and access
 * - Bounding box computation
 * - Parent-child relationships for topology hierarchy
 *
 * Thread-safety: Read operations are thread-safe. Modifications should be
 * synchronized externally.
 */
class GeometryEntity : public std::enable_shared_from_this<GeometryEntity>,
                       public Kangaroo::Util::NonCopyMoveable {
public:
    /**
     * @brief Default constructor creates an empty entity
     */
    GeometryEntity();

    /**
     * @brief Construct entity from OCC shape
     * @param shape The OpenCASCADE shape to wrap
     * @param type Optional explicit entity type (auto-detected if not provided)
     */
    explicit GeometryEntity(const TopoDS_Shape& shape, EntityType type = EntityType::None);

    /// Virtual destructor for proper inheritance
    virtual ~GeometryEntity() = default;

    // -------------------------------------------------------------------------
    // ID Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Get the global unique entity ID
     * @return Globally unique EntityId
     */
    [[nodiscard]] EntityId entityId() const { return m_entityId; }

    /**
     * @brief Get the type-scoped unique ID
     * @return EntityUID unique within this entity type
     */
    [[nodiscard]] EntityUID entityUID() const { return m_entityUID; }

    /**
     * @brief Get the entity type
     * @return EntityType enumeration value
     */
    [[nodiscard]] EntityType entityType() const { return m_entityType; }

    // -------------------------------------------------------------------------
    // Shape Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Get the underlying OCC shape
     * @return Const reference to TopoDS_Shape
     */
    [[nodiscard]] const TopoDS_Shape& shape() const { return m_shape; }

    /**
     * @brief Check if entity has a valid shape
     * @return true if shape is not null
     */
    [[nodiscard]] bool hasShape() const { return !m_shape.IsNull(); }

    // -------------------------------------------------------------------------
    // Geometry Properties
    // -------------------------------------------------------------------------

    /**
     * @brief Compute or get cached bounding box
     * @return Axis-aligned bounding box
     */
    [[nodiscard]] BoundingBox3D boundingBox() const;

    /**
     * @brief Check if bounding box has been computed
     * @return true if bounding box is available
     */
    [[nodiscard]] bool hasBoundingBox() const { return m_boundingBoxValid; }

    /**
     * @brief Invalidate cached bounding box (force recomputation)
     */
    void invalidateBoundingBox();

    // -------------------------------------------------------------------------
    // Hierarchy
    // -------------------------------------------------------------------------

    /**
     * @brief Get parent entity (if any)
     * @return Weak pointer to parent, or empty if root
     */
    [[nodiscard]] GeometryEntityWeakPtr parent() const { return m_parent; }

    /**
     * @brief Get child entities
     * @return Vector of shared pointers to children
     */
    [[nodiscard]] const std::vector<GeometryEntityPtr>& children() const { return m_children; }

    /**
     * @brief Add a child entity
     * @param child Entity to add as child
     */
    void addChild(const GeometryEntityPtr& child);

    /**
     * @brief Remove a child entity
     * @param child Entity to remove
     * @return true if child was found and removed
     */
    bool removeChild(const GeometryEntityPtr& child);

    /**
     * @brief Set parent entity
     * @param parent New parent entity
     */
    void setParent(const GeometryEntityWeakPtr& parent) { m_parent = parent; }

    // -------------------------------------------------------------------------
    // Name/Label
    // -------------------------------------------------------------------------

    /**
     * @brief Get entity display name
     * @return User-visible name string
     */
    [[nodiscard]] const std::string& name() const { return m_name; }

    /**
     * @brief Set entity display name
     * @param name New display name
     */
    void setName(const std::string& name) { m_name = name; }

protected:
    /**
     * @brief Compute bounding box from OCC shape
     */
    void computeBoundingBox() const;

    /**
     * @brief Detect entity type from OCC shape
     * @param shape Shape to analyze
     * @return Detected EntityType
     */
    [[nodiscard]] static EntityType detectEntityType(const TopoDS_Shape& shape);

protected:
    EntityId m_entityId{INVALID_ENTITY_ID};    ///< Global unique ID
    EntityUID m_entityUID{INVALID_ENTITY_UID}; ///< Type-scoped unique ID
    EntityType m_entityType{EntityType::None}; ///< Entity type

    TopoDS_Shape m_shape; ///< Underlying OCC shape

    mutable BoundingBox3D m_boundingBox;    ///< Cached bounding box
    mutable bool m_boundingBoxValid{false}; ///< Bounding box validity flag

    GeometryEntityWeakPtr m_parent;            ///< Parent entity reference
    std::vector<GeometryEntityPtr> m_children; ///< Child entities

    std::string m_name; ///< Display name
};

} // namespace OpenGeoLab::Geometry