/**
 * @file geometry_entity.hpp
 * @brief Geometry entity management with OCC integration
 *
 * This file provides the core geometry entity classes that wrap OpenCASCADE
 * topological shapes and provide ID-based entity management. The entity system
 * supports both global EntityId and type-scoped EntityUID for flexible querying.
 */

#pragma once

#include "geometry/geometry_entity.hpp"
#include "geometry/geometry_types.hpp"
#include <kangaroo/util/noncopyable.hpp>

#include <memory>
#include <string>

class TopoDS_Shape;

namespace OpenGeoLab::Geometry {
class GeometryEntityImpl;
class GeometryManager;
class GeometryDocumentImpl;

using GeometryEntityImplPtr = std::shared_ptr<GeometryEntityImpl>;
using GeometryEntityImplWeakPtr = std::weak_ptr<GeometryEntityImpl>;

// =============================================================================
// GeometryEntityImpl
// =============================================================================
/**
 * @brief Base class for all geometry entities wrapping OCC shapes
 *
 * GeometryEntityImpl provides:
 * - Dual ID system (EntityId for global, EntityUID for type-scoped)
 * - OCC TopoDS_Shape storage and access
 * - Bounding box computation
 * - Parent-child relationships for topology hierarchy
 *
 * Thread-safety: Read operations are thread-safe. Modifications should be
 * synchronized externally.
 */
class GeometryEntityImpl : public std::enable_shared_from_this<GeometryEntityImpl>,
                           public GeometryEntity {
public:
    /**
     * @brief Destructor.
     *
     * Ensures parent/child edges are detached in a best-effort manner.
     *
     * @note Edges are normally detached eagerly by GeometryDocument/EntityRelationshipIndex
     *       on entity removal. This destructor acts as a defensive fallback
     *       for cases where an entity outlives its document via external
     *       shared ownership.
     */
    virtual ~GeometryEntityImpl();
    // -------------------------------------------------------------------------
    // Type Information
    // -------------------------------------------------------------------------

    /**
     * @brief Get the entity type
     * @return EntityType enumeration value
     */
    [[nodiscard]] virtual EntityType entityType() const override { return m_entityType; };

    // -------------------------------------------------------------------------
    // ID Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Get the global unique entity ID
     * @return Globally unique EntityId
     */
    [[nodiscard]] virtual EntityId entityId() const override { return m_entityId; }

    /**
     * @brief Get the type-scoped unique ID
     * @return EntityUID unique within this entity type
     */
    [[nodiscard]] virtual EntityUID entityUID() const override { return m_entityUID; }

    /**
     * @brief Get an EntityKey handle for this entity
     * @return EntityKey consisting of (EntityId, EntityUID, EntityType)
     */
    [[nodiscard]] virtual EntityKey entityKey() const override {
        return EntityKey(entityId(), entityUID(), entityType());
    }

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
    // Hierarchy Management
    // -------------------------------------------------------------------------

    /**
     * @brief Check whether a parent->child edge is allowed by type.
     * @param child_type Type of the prospective child.
     * @return true if this entity is allowed to have a child of the given type.
     * @note This is a pure type-level constraint. Document presence and entity
     *       existence are validated by GeometryDocument when creating edges.
     */
    [[nodiscard]] virtual bool canAddChildType(EntityType child_type) const = 0;

    /**
     * @brief Check whether a parent->child edge is allowed by type.
     * @param parent_type Type of the prospective parent.
     * @return true if this entity is allowed to have a parent of the given type.
     */
    [[nodiscard]] virtual bool canAddParentType(EntityType parent_type) const = 0;

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
     * @brief Protected constructor for derived classes
     * @param type Entity type for UID generation
     */
    explicit GeometryEntityImpl(EntityType type);

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
    EntityType m_entityType{EntityType::None}; ///< Cached entity type

    mutable BoundingBox3D m_boundingBox;    ///< Cached bounding box
    mutable bool m_boundingBoxValid{false}; ///< Bounding box validity

    friend class EntityIndex;
    friend class GeometryDocumentImpl;

    // Set/clear by EntityIndex on add/remove; non-owning.
    void setDocument(std::weak_ptr<GeometryDocumentImpl> document) {
        m_document = std::move(document);
    }
    [[nodiscard]] std::shared_ptr<GeometryDocumentImpl> document() const {
        return m_document.lock();
    }

    // Called to detach edges from the document relationship index.
    void detachAllRelations();

    std::weak_ptr<GeometryDocumentImpl> m_document;

    std::string m_name; ///< Display name
};

} // namespace OpenGeoLab::Geometry