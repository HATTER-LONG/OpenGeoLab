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

// =============================================================================
// GeometryManager
// =============================================================================

/**
 * @brief Central manager for geometry entities with ID-based lookup
 *
 * GeometryManager provides:
 * - Registration and tracking of all geometry entities
 * - Fast lookup by EntityId or by (EntityType, EntityUID) pair
 * - OCC shape to entity mapping
 * - Iteration over entities by type
 *
 * Thread-safety: All operations are NOT thread-safe. External synchronization
 * is required for concurrent access.
 */
class GeometryManager : public Kangaroo::Util::NonCopyMoveable {
public:
    GeometryManager() = default;
    ~GeometryManager() = default;

    // -------------------------------------------------------------------------
    // Entity Registration
    // -------------------------------------------------------------------------

    /**
     * @brief Register an entity with the manager
     * @param entity Entity to register
     * @note Entity must have valid IDs before registration
     */
    void registerEntity(const GeometryEntityPtr& entity);

    /**
     * @brief Unregister an entity from the manager
     * @param entity Entity to unregister
     * @return true if entity was found and removed
     */
    bool unregisterEntity(const GeometryEntityPtr& entity);

    /**
     * @brief Clear all registered entities
     */
    void clear();

    // -------------------------------------------------------------------------
    // Entity Lookup
    // -------------------------------------------------------------------------

    /**
     * @brief Find entity by global ID
     * @param id Global EntityId
     * @return Entity pointer or nullptr if not found
     */
    [[nodiscard]] GeometryEntityPtr findById(EntityId id) const;

    /**
     * @brief Find entity by type and type-scoped UID
     * @param type Entity type
     * @param uid Type-scoped EntityUID
     * @return Entity pointer or nullptr if not found
     */
    [[nodiscard]] GeometryEntityPtr findByTypeAndUID(EntityType type, EntityUID uid) const;

    /**
     * @brief Find entity by OCC shape
     * @param shape OCC shape to look up
     * @return Entity pointer or nullptr if not found
     * @note Uses shape hash for lookup; may have collisions
     */
    [[nodiscard]] GeometryEntityPtr findByShape(const TopoDS_Shape& shape) const;

    // -------------------------------------------------------------------------
    // Entity Enumeration
    // -------------------------------------------------------------------------

    /**
     * @brief Get all entities of a specific type
     * @param type Entity type to filter by
     * @return Vector of matching entities
     */
    [[nodiscard]] std::vector<GeometryEntityPtr> getEntitiesByType(EntityType type) const;

    /**
     * @brief Get all registered entities
     * @return Vector of all entities
     */
    [[nodiscard]] std::vector<GeometryEntityPtr> getAllEntities() const;

    /**
     * @brief Get total entity count
     * @return Number of registered entities
     */
    [[nodiscard]] size_t entityCount() const { return m_entitiesById.size(); }

    /**
     * @brief Get entity count by type
     * @param type Entity type to count
     * @return Number of entities of the specified type
     */
    [[nodiscard]] size_t entityCountByType(EntityType type) const;

    // -------------------------------------------------------------------------
    // Shape Creation Helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Create and register an entity from an OCC shape
     * @param shape OCC shape to wrap
     * @param parent Optional parent entity
     * @return Newly created and registered entity
     */
    GeometryEntityPtr createEntityFromShape(const TopoDS_Shape& shape,
                                            const GeometryEntityPtr& parent = nullptr);

    /**
     * @brief Import a compound shape and create entity hierarchy
     * @param shape Root shape to import
     * @return Root entity of the imported hierarchy
     *
     * This method traverses the OCC shape hierarchy and creates
     * corresponding GeometryEntity objects with proper parent-child
     * relationships.
     */
    GeometryEntityPtr importShape(const TopoDS_Shape& shape);

private:
    /**
     * @brief Recursively create entities from shape hierarchy
     * @param shape Current shape
     * @param parent Parent entity
     * @return Created entity
     */
    GeometryEntityPtr importShapeRecursive(const TopoDS_Shape& shape,
                                           const GeometryEntityPtr& parent);

    /**
     * @brief Compute hash key for shape lookup
     * @param shape Shape to hash
     * @return Hash value
     */
    [[nodiscard]] static size_t computeShapeHash(const TopoDS_Shape& shape);

private:
    /// Primary lookup: EntityId -> Entity
    std::unordered_map<EntityId, GeometryEntityPtr> m_entitiesById;

    /// Secondary lookup: (Type, UID) -> Entity
    /// Key is computed as (type << 56) | uid
    std::unordered_map<uint64_t, GeometryEntityPtr> m_entitiesByTypeUID;

    /// OCC shape hash -> Entity (for reverse lookup)
    std::unordered_map<size_t, GeometryEntityPtr> m_entitiesByShapeHash;
};

} // namespace OpenGeoLab::Geometry