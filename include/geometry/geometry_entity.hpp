/**
 * @file geometry_entity.hpp
 * @brief Abstract interface for geometry entities in the entity hierarchy
 *
 * Defines the public GeometryEntity interface used across module boundaries.
 * Concrete implementations reside in src/geometry/entity/.
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include <TopoDS_Shape.hxx>
#include <kangaroo/util/noncopyable.hpp>
#include <memory>

namespace OpenGeoLab::Geometry {
class GeometryEntity;

using GeometryEntityPtr = std::shared_ptr<GeometryEntity>;

/**
 * @brief Abstract interface for all geometry entities
 *
 * Provides read-only access to entity identity (ID, UID, type, key)
 * and the underlying OCC shape. Concrete subclasses (PartEntity, FaceEntity, etc.)
 * add shape-specific accessors.
 */
class GeometryEntity : public Kangaroo::Util::NonCopyMoveable {
public:
    virtual ~GeometryEntity() = default;
    // -------------------------------------------------------------------------
    // Type Information
    // -------------------------------------------------------------------------
    /**
     * @brief Get the global unique entity ID
     * @return Globally unique EntityId
     */
    [[nodiscard]] virtual EntityId entityId() const = 0;
    /**
     * @brief Get the type-scoped unique ID
     * @return EntityUID unique within this entity type
     */
    [[nodiscard]] virtual EntityUID entityUID() const = 0;
    /**
     * @brief Get the entity type
     * @return EntityType enumeration value
     */
    [[nodiscard]] virtual EntityType entityType() const = 0;
    /**
     * @brief Get an EntityKey handle for this entity
     * @return EntityKey consisting of (EntityId, EntityUID, EntityType)
     */
    [[nodiscard]] virtual EntityKey entityKey() const = 0;

    // -------------------------------------------------------------------------
    // Shape Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Get the underlying OCC shape
     * @return Const reference to TopoDS_Shape
     */
    [[nodiscard]] virtual const TopoDS_Shape& shape() const = 0;

    /**
     * @brief Check if entity has a valid shape
     * @return true if shape is not null
     */
    [[nodiscard]] virtual bool hasShape() const = 0;
};
} // namespace OpenGeoLab::Geometry